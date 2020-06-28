/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "AutonWriter.h"
#include <iostream>
#include "SignalSet.h"
#include "FileNamer.h"
#include "Hdf5Writer.h"
#include "json.hpp"
#include "config.h"
#include "SignalData.h"

namespace FormatConverter{

  AutonWriter::AutonWriter( ) : Writer( "autonlab" ) { }

  AutonWriter::~AutonWriter( ) { }

  int AutonWriter::drain( SignalSet * info ) {
    dataptr = info;
    return 0;
  }

  std::vector<std::string> AutonWriter::closeDataSet( ) {
    //auto firstTime = dataptr->earliest( );
    auto lastTime = dataptr->latest( );
    std::vector<std::string> ret;
    if ( 0 == lastTime ) {
      // we don't have any data at all!
      return ret;
    }

    std::string outy = filenamer( ).filename( dataptr );

    if ( dataptr->vitals( ).empty( ) && dataptr->waves( ).empty( ) ) {
      std::cerr << "Nothing to write to " << outy << std::endl;
      return ret;
    }

    output( ) << "Writing to " << outy << std::endl;

    H5::Exception::dontPrint( );
    try {
      H5::H5File file( outy, H5F_ACC_TRUNC );
      writeGlobalMetas( file );

      auto numerics = Hdf5Writer::ensureGroupExists( file, "Numerics" );
      auto vitals = Hdf5Writer::ensureGroupExists( numerics, "Vitals" );

      auto vitnames = std::vector<std::string>{ };
      for ( auto& v : dataptr->vitals( ) ) {
        vitnames.push_back( v->name( ) );
        writeVital( vitals, v );
      }

      auto waves = Hdf5Writer::ensureGroupExists( file, "Waveforms" );
      auto hemos = Hdf5Writer::ensureGroupExists( waves, "Hemodynamics" );

      auto wavnames = std::vector<std::string>{ };
      for ( auto& v : dataptr->waves( ) ) {
        wavnames.push_back( v->name( ) );
        writeWave( hemos, v );
      }

      //      createTimeCache( );
      //      createEventsAndTimes( file, dataptr ); // also creates the timesteplkp
      //
      //      auto auxdata = dataptr->auxdata( );
      //      if ( !auxdata.empty( ) ) {
      //        for ( auto& fileaux : auxdata ) {
      //          writeAuxData( file, fileaux.first, fileaux.second );
      //        }
      //      }
      //
      //      writeFileAttributes( file, dataptr->metadata( ), firstTime, lastTime );
      //
      //      auto grp = ensureGroupExists( file, "VitalSigns" );
      //      output( ) << "Writing " << dataptr->vitals( ).size( ) << " Vitals" << std::endl;
      //      for ( auto& vits : dataptr->vitals( ) ) {
      //        if ( vits->empty( ) ) {
      //          output( ) << "Skipping Vital: " << vits->name( ) << "(no data)" << std::endl;
      //        }
      //        else {
      //          auto g = ensureGroupExists( grp, getDatasetName( vits ) );
      //          writeVitalGroup( g, vits );
      //        }
      //      }
      //
      //      if ( !this->skipwaves( ) ) {
      //        grp = ensureGroupExists( file, "Waveforms" );
      //        output( ) << "Writing " << dataptr->waves( ).size( ) << " Waveforms" << std::endl;
      //        for ( auto& wavs : dataptr->waves( ) ) {
      //          if ( wavs->empty( ) ) {
      //            output( ) << "Skipping Wave: " << wavs->name( ) << "(no data)" << std::endl;
      //          }
      //          else {
      //            H5::Group g = ensureGroupExists( grp, getDatasetName( wavs ) );
      //            writeWaveGroup( g, wavs );
      //          }
      //        }
      //      }

      ret.push_back( outy );
    }
    // catch failure caused by the H5File operations
    catch ( H5::FileIException& error ) {
      output( ) << "error writing file: " << error.getFuncName( ) << " said " << error.getDetailMsg( ) << std::endl;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      output( ) << "error writing dataset: " << error.getFuncName( ) << " said " << error.getDetailMsg( ) << std::endl;
    }
    // catch failure caused by the DataSpace operations
    catch ( H5::DataSpaceIException& error ) {
      output( ) << "error writing dataset: " << error.getFuncName( ) << " said " << error.getDetailMsg( ) << std::endl;
    }
    catch ( H5::GroupIException& error ) {
      output( ) << "error writing group: " << error.getFuncName( ) << " said " << error.getDetailMsg( ) << std::endl;
    }

    return ret;
  }

  void AutonWriter::writeGlobalMetas( H5::H5File& file ) {

    Hdf5Writer::writeAttribute( file, "format", "CCDEF" );
    Hdf5Writer::writeAttribute( file, "version", "1.1" );

    auto globalmetas = Hdf5Writer::ensureGroupExists( file, ".meta" );

    auto audata = nlohmann::json{ };
    audata["version"] = GIT_BUILD;
    audata["data_version"] = 1;
    Hdf5Writer::writeAttribute( globalmetas, "audata", audata.dump( 2, ' ', true ) );

    auto data = nlohmann::json{ };
    data["time"]["origin"] = dataptr->earliest( );
    data["time"]["units"] = "ms";
    data["time"]["tz"] = "GMT";
    Hdf5Writer::writeAttribute( globalmetas, "data", data.dump( 2, ' ', true ) );
  }

  void AutonWriter::writeVital( H5::H5Object& loc, SignalData * signal ) {
    auto dsname = Hdf5Writer::getDatasetName( signal->name( ) );

    const hsize_t rows = signal->size( );

    hsize_t dims[] = { rows, 1 };
    auto space = H5::DataSpace{ 2, dims };

    H5::DSetCreatPropList props;
    if ( compression( ) > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      Hdf5Writer::autochunk( dims, 2, sizeof ( double ), chunkdims );
      props.setChunk( 2, chunkdims );
      props.setShuffle( );
      props.setDeflate( compression( ) );
    }

    auto ds = loc.createDataSet( dsname, H5::PredType::IEEE_F64LE, space, props );

    const hsize_t maxslabcnt = ( rows > 125000
        ? 125000
        : rows );
    hsize_t offset[] = { 0, 0 };
    hsize_t count[] = { 0, 1 };

    auto buffer = std::vector<double>{ };
    buffer.reserve( maxslabcnt );

    // We're keeping a buffer to eventually write to the file. However, this
    // buffer is based on a number of rows, *not* some multiple of the data size.
    // This means we are really keeping two counters going at all times, and
    // once we're out of all the loops, we need to write any residual data.
    size_t rowcount = 0;
    for ( size_t row = 0; row < rows; row++ ) {
      auto datarow = signal->pop( );
      buffer.push_back( datarow->doubles( )[0] );

      if ( ++rowcount >= maxslabcnt ) {
        offset[0] += count[0];
        count[0] = rowcount;
        space.selectHyperslab( H5S_SELECT_SET, count, offset );

        H5::DataSpace memspace( 2, count );
        ds.write( buffer.data( ), H5::PredType::IEEE_F64LE, memspace, space );
        buffer.clear( );
        buffer.reserve( maxslabcnt );
        rowcount = 0;
      }
    }

    // finally, write whatever's left in the buffer
    if ( buffer.empty( ) ) {
      offset[0] += count[0];
      count[0] = rowcount;
      space.selectHyperslab( H5S_SELECT_SET, count, offset );
      H5::DataSpace memspace( 2, count );
      ds.write( buffer.data( ), H5::PredType::IEEE_F64LE, memspace, space );
    }
  }

  void AutonWriter::writeWave( H5::H5Object& loc, SignalData * signal ) {
    auto dsname = Hdf5Writer::getDatasetName( signal->name( ) );
    const hsize_t rows = signal->size( );
    const int valsperrow = signal->readingsPerChunk( );

    hsize_t dims[] = { rows * valsperrow, 1 };
    auto space = H5::DataSpace{ 2, dims };
    H5::DSetCreatPropList props;
    if ( compression( ) > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      Hdf5Writer::autochunk( dims, 2, sizeof ( short ), chunkdims );
      props.setChunk( 2, chunkdims );
      props.setShuffle( );
      props.setDeflate( compression( ) );
    }

    H5::DataSet ds = loc.createDataSet( dsname, H5::PredType::IEEE_F64LE, space, props );

    const hsize_t maxslabcnt = ( rows * valsperrow > 125000 ? 125000 : rows * valsperrow );
    hsize_t offset[] = { 0, 0 };
    hsize_t count[] = { 0, 1 };

    auto buffer = std::vector<double>{ };
    buffer.reserve( maxslabcnt );

    // We're keeping a buffer to eventually write to the file. However, this
    // buffer is based on a number of rows, *not* some multiple of the data size.
    // This means we are really keeping two counters going at all times, and
    // once we're out of all the loops, we need to write any residual data.

    for ( size_t row = 0; row < rows; row++ ) {
      auto datarow = signal->pop( );
      auto dbls = datarow->doubles( );
      buffer.insert( buffer.end( ), dbls.begin( ), dbls.end( ) );


      if ( buffer.size( ) >= maxslabcnt ) {
        count[0] = buffer.size( );
        space.selectHyperslab( H5S_SELECT_SET, count, offset );
        offset[0] += count[0];

        auto memspace = H5::DataSpace{ 2, count };
        ds.write( buffer.data( ), H5::PredType::IEEE_F64LE, memspace, space );
        buffer.clear( );
        buffer.reserve( maxslabcnt );
      }
    }

    // finally, write whatever's left in the buffer
    if ( !buffer.empty( ) ) {
      count[0] = buffer.size( );
      space.selectHyperslab( H5S_SELECT_SET, count, offset );
      auto memspace = H5::DataSpace{ 2, count };
      ds.write( buffer.data( ), H5::PredType::IEEE_F64LE, memspace, space );
    }
  }
}
