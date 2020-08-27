/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <iostream>

#include "AutonWriter.h"
#include "SignalSet.h"
#include "FileNamer.h"
#include "Hdf5Writer.h"
#include "json.hpp"
#include "config.h"
#include "SignalData.h"
#include "Log.h"
#include "Options.h"

namespace FormatConverter{

  AutonWriter::AutonWriter( ) : Writer( "au" ) { }

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

    Log::info( ) << "Writing to " << outy << std::endl;

    H5::Exception::dontPrint( );
    try {
      H5::H5File file( outy, H5F_ACC_TRUNC );
      writeGlobalMetas( file );

      auto topdata = Hdf5Writer::ensureGroupExists( file, "data" );
      auto vitals = Hdf5Writer::ensureGroupExists( topdata, "vitals" );

      auto vitnames = std::vector<std::string>{ };
      for ( auto& v : dataptr->vitals( ) ) {
        vitnames.push_back( v->name( ) );
        writeVital( vitals, v );
      }

      auto waves = Hdf5Writer::ensureGroupExists( topdata, "waveforms" );
      auto wavnames = std::vector<std::string>{ };
      if ( !this->skipwaves( ) ) {
        for ( auto& v : dataptr->waves( ) ) {
          wavnames.push_back( v->name( ) );
          writeWave( waves, v );
        }
      }

      ret.push_back( outy );
    }
    // catch failure caused by the H5File operations
    catch ( H5::FileIException& error ) {
      Log::error( ) << "error writing file: " << error.getFuncName( ) << " said " << error.getDetailMsg( ) << std::endl;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      Log::error( ) << "error writing dataset: " << error.getFuncName( ) << " said " << error.getDetailMsg( ) << std::endl;
    }
    // catch failure caused by the DataSpace operations
    catch ( H5::DataSpaceIException& error ) {
      Log::error( ) << "error writing dataset: " << error.getFuncName( ) << " said " << error.getDetailMsg( ) << std::endl;
    }
    catch ( H5::GroupIException& error ) {
      Log::error( ) << "error writing group: " << error.getFuncName( ) << " said " << error.getDetailMsg( ) << std::endl;
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


    auto mapping = nlohmann::json{ };
    const auto& source = dataptr->metadata( ).at( "Source Reader" );

    for ( auto& signal : dataptr->allsignals( ) ) {
      const auto& name = signal->name( );

      if ( !signal->uom( ).empty( ) ) {
        mapping["common_parameters"][name]["UOM"] = signal->uom( );
      }

      if ( !source.empty( ) ) {
        mapping["common_parameters"][name]["data_source"] = source;
      }

      auto dsname = Hdf5Writer::getDatasetName( name );
      mapping["common_parameters"][name]["data_location"] = ( signal->wave( )
          ? "/data/waveforms/" + dsname
          : "/data/vitals/" + dsname );
    }
    Hdf5Writer::writeAttribute( globalmetas, "mapping", mapping.dump( 2, ' ', true ) );
  }

  void AutonWriter::writeMetas( H5::H5Object& loc, SignalData * signal ) {
    auto meta = nlohmann::json{ };
    meta["columns"]["time"]["type"] = "time";
    meta["columns"]["value"]["type"] = ( Options::asBool( OptionsKey::INDEXED_TIME )
        ? "indexed"
        : "real" );

    for ( const auto& it : signal->metad( ) ) {
      meta["properties"][it.first] = it.second;
    }
    for ( const auto& it : signal->metas( ) ) {
      if ( !( "Note on Scale" == it.first || "Note on Min/Max" == it.first ) ) {
        meta["properties"][it.first] = it.second;
      }
    }
    for ( const auto& it : signal->metai( ) ) {
      if ( "Scale" != it.first ) {
        meta["properties"][it.first] = it.second;
      }
    }

    Hdf5Writer::writeAttribute( loc, ".meta", meta.dump( 2, ' ', true ) );
  }

  void AutonWriter::writeVital( H5::H5Object& loc, SignalData * signal ) {
    Log::debug( ) << "writing vital: " << signal->name( ) << std::endl;
    auto dsname = Hdf5Writer::getDatasetName( signal->name( ) );

    auto CMPDTYPE = H5::CompType( sizeof (autondata ) );
    CMPDTYPE.insertMember( "Time", 0, H5::PredType::IEEE_F64LE );
    CMPDTYPE.insertMember( signal->name( ), sizeof (double ), H5::PredType::IEEE_F64LE );

    const hsize_t rows = signal->size( );

    hsize_t dims[] = { rows, 1 };
    auto space = H5::DataSpace{ 1, dims };

    H5::DSetCreatPropList props;
    if ( compression( ) > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      Hdf5Writer::autochunk( dims, 1, sizeof ( autondata ), chunkdims );
      props.setChunk( 1, chunkdims );
      props.setShuffle( );
      props.setDeflate( compression( ) );
    }

    auto ds = loc.createDataSet( dsname, CMPDTYPE, space, props );
    writeMetas( ds, signal );

    const hsize_t maxslabcnt = std::min( rows, (hsize_t) ( 1024 * 256 ) );
    hsize_t offset[] = { 0, 0 };
    hsize_t count[] = { 0, 1 };

    auto buffer = std::vector<autondata>{ };
    buffer.reserve( maxslabcnt );

    // We're keeping a buffer to eventually write to the file. However, this
    // buffer is based on a number of rows, *not* some multiple of the data size.
    // This means we are really keeping two counters going at all times, and
    // once we're out of all the loops, we need to write any residual data.
    size_t rowcount = 0;
    for ( size_t row = 0; row < rows; row++ ) {
      auto datarow = signal->pop( );
      buffer.push_back( autondata{
        static_cast<double> ( datarow->time ),
        datarow->doubles( )[0]
      } );

      if ( ++rowcount >= maxslabcnt ) {
        offset[0] += count[0];
        count[0] = rowcount;
        space.selectHyperslab( H5S_SELECT_SET, count, offset );

        H5::DataSpace memspace( 1, count );
        Log::debug( ) << "writing " << rowcount << " data points in a slab" << std::endl;
        ds.write( buffer.data( ), CMPDTYPE, memspace, space );
        buffer.clear( );
        buffer.reserve( maxslabcnt );
        rowcount = 0;
      }
    }

    // finally, write whatever's left in the buffer
    if ( !buffer.empty( ) ) {
      offset[0] += count[0];
      count[0] = rowcount;
      space.selectHyperslab( H5S_SELECT_SET, count, offset );
      H5::DataSpace memspace( 2, count );
      Log::debug( ) << "writing (leftovers) " << rowcount << " data points in a slab" << std::endl;
      ds.write( buffer.data( ), CMPDTYPE, memspace, space );
    }
  }

  void AutonWriter::writeWave( H5::H5Object& loc, SignalData * signal ) {
    Log::debug( ) << "writing wave: " << signal->name( ) << std::endl;
    auto dsname = Hdf5Writer::getDatasetName( signal->name( ) );
    const hsize_t rows = signal->size( );
    const auto valsperrow = signal->readingsPerChunk( );
    const auto msstep = signal->chunkInterval( ) / valsperrow;

    auto CMPDTYPE = H5::CompType( sizeof (autondata ) );
    CMPDTYPE.insertMember( "Time", 0, H5::PredType::IEEE_F64LE );
    CMPDTYPE.insertMember( signal->name( ), sizeof (double ), H5::PredType::IEEE_F64LE );

    hsize_t dims[] = { rows * valsperrow, 1 };
    auto space = H5::DataSpace{ 1, dims };
    H5::DSetCreatPropList props;
    if ( compression( ) > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      Hdf5Writer::autochunk( dims, 1, sizeof ( autondata ), chunkdims );
      props.setChunk( 1, chunkdims );
      props.setShuffle( );
      props.setDeflate( compression( ) );
    }

    H5::DataSet ds = loc.createDataSet( dsname, CMPDTYPE, space, props );
    writeMetas( ds, signal );

    // this is a "soft" max value...the actual max is based on this
    // signal's valsperrow
    const hsize_t maxslabcnt = std::min( rows * valsperrow, (hsize_t) 1024 * 256 );
    hsize_t offset[] = { 0, 0 };
    hsize_t count[] = { 0, 1 };

    auto buffer = std::vector<autondata>{ };
    buffer.reserve( maxslabcnt );

    // We're keeping a buffer to eventually write to the file. However, this
    // buffer is based on a number of rows, *not* some multiple of the data size.
    // This means we are really keeping two counters going at all times, and
    // once we're out of all the loops, we need to write any residual data.

    for ( size_t row = 0; row < rows; row++ ) {
      auto datarow = signal->pop( );
      auto dbls = datarow->doubles( );
      auto rowtime = datarow->time;

      for ( size_t vidx = 0; vidx < dbls.size( ); vidx++ ) {
        buffer.push_back({
          static_cast<double> ( rowtime ),
          dbls[vidx]
        } );
        rowtime += msstep;
      }

      // we have two values per row, so our buffer is actually 2X big
      if ( buffer.size( ) >= maxslabcnt ) {
        Log::debug( ) << "writing " << buffer.size( ) << " data points in a slab" << std::endl;

        count[0] = buffer.size( );
        space.selectHyperslab( H5S_SELECT_SET, count, offset );
        offset[0] += count[0];
        //Log::debug( ) << "writing slab " << x++ << "\tcount: " << count[0] << "\toffset: " << offset[0] << std::endl;

        auto memspace = H5::DataSpace{ 1, count };
        ds.write( buffer.data( ), CMPDTYPE, memspace, space );
        buffer.clear( );
        buffer.reserve( maxslabcnt );
      }
    }

    // finally, write whatever's left in the buffer
    if ( !buffer.empty( ) ) {
      Log::debug( ) << "writing (leftovers) " << buffer.size( ) << " values in a slab" << std::endl;
      count[0] = buffer.size( );
      space.selectHyperslab( H5S_SELECT_SET, count, offset );
      auto memspace = H5::DataSpace{ 1, count };
      ds.write( buffer.data( ), CMPDTYPE, memspace, space );
    }
  }
}
