/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Hdf5Writer.cpp
 * Author: ryan
 *
 * Created on August 26, 2016, 12:58 PM
 */

#include <iostream>
#include <utility>
#include <chrono>
#include <sstream>
#include <limits>
#include <cmath>

#include "config.h"
#include "SignalSet.h"
#include "Hdf5Writer.h"
#include "SignalData.h"
#include "DataRow.h"
#include "SignalUtils.h"
#include "FileNamer.h"
#include "H5public.h"
#include "TimezoneOffsetTimeSignalSet.h"
#include "Options.h"
#include "TimeRange.h"
#include "Log.h"

namespace FormatConverter{
  const std::string Hdf5Writer::LAYOUT_VERSION = "4.1.1";

  Hdf5Writer::Hdf5Writer( ) : Writer( "hdf5" ) { }

  Hdf5Writer::Hdf5Writer( const Hdf5Writer& orig ) : Writer( "hdf5" ) { }

  Hdf5Writer::~Hdf5Writer( ) { }

  void Hdf5Writer::writeAttribute( H5::H5Object& loc,
      const std::string& attr, const std::string& val ) {
    if ( !val.empty( ) ) {
      try {
        Log::trace( ) << "writing attribute (string):" << loc.getObjName( ) << " " << attr << ": " << val << std::endl;

        H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
        H5::StrType st( H5::PredType::C_S1, H5T_VARIABLE );
        st.setCset( H5T_CSET_UTF8 );
        H5::Attribute attrib = loc.createAttribute( attr, st, space );
        attrib.write( st, val );
      }
      catch ( H5::AttributeIException& x ) {
        Log::error( ) << x.getDetailMsg( ) << std::endl;
      }
    }
  }

  void Hdf5Writer::writeAttribute( H5::H5Object& loc, const std::string& attr, int val ) {
    Log::trace( ) << "writing attribute (int):" << loc.getObjName( ) << " " << attr << ": " << val << std::endl;

    H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
    H5::Attribute attrib = loc.createAttribute( attr, H5::PredType::STD_I32LE, space );
    attrib.write( H5::PredType::STD_I32LE, &val );
  }

  void Hdf5Writer::writeAttribute( H5::H5Object& loc, const std::string& attr, dr_time val ) {
    Log::trace( ) << "writing attribute (time):" << loc.getObjName( ) << " " << attr << ": " << val << std::endl;

    H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
    H5::Attribute attrib = loc.createAttribute( attr, H5::PredType::STD_I64LE, space );
    attrib.write( H5::PredType::STD_I64LE, &val );
  }

  void Hdf5Writer::writeAttribute( H5::H5Object& loc, const std::string& attr, double val ) {
    Log::trace( ) << "writing attribute (double):" << loc.getObjName( ) << " " << attr << ": " << val << std::endl;

    H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
    H5::Attribute attrib = loc.createAttribute( attr, H5::PredType::IEEE_F64LE, space );
    attrib.write( H5::PredType::IEEE_F64LE, &val );
  }

  void Hdf5Writer::writeTimesAndDurationAttributes( H5::H5Object& loc, const dr_time& start, const dr_time& end ) {
    Log::trace( ) << "writing times and durations for " << loc.getObjName( ) << std::endl;

    const bool indexedtime = FormatConverter::Options::asBool( FormatConverter::OptionsKey::INDEXED_TIME );

    time_t stime;
    time_t etime;
    if ( indexedtime ) {
      stime = timesteplkp.at( start );
      etime = timesteplkp.at( end );

      writeAttribute( loc, SignalData::STARTTIME, stime );
      writeAttribute( loc, SignalData::ENDTIME, etime );
    }
    else {
      stime = ( start / 1000 );
      etime = ( end / 1000 );

      writeAttribute( loc, SignalData::STARTTIME, start );
      writeAttribute( loc, SignalData::ENDTIME, end );
    }

    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime( buf, sizeof buf, "%FT%TZ", gmtime( &stime ) );
    writeAttribute( loc, "Start Date/Time", buf );

    strftime( buf, sizeof buf, "%FT%TZ", gmtime( &etime ) );
    writeAttribute( loc, "End Date/Time", buf );

    time_t xx( etime - stime );
    tm * t = gmtime( &xx );

    std::string duration = "";
    if ( t->tm_yday ) {
      duration += std::to_string( t->tm_yday ) + ( t->tm_yday > 1 ? " days, " : " day, " );
    }
    if ( t->tm_hour < 10 ) {
      duration += "0";
    }
    duration += std::to_string( t->tm_hour ) + ":";

    if ( t->tm_min < 10 ) {
      duration += "0";
    }
    duration += std::to_string( t->tm_min ) + ":";

    if ( t->tm_sec < 10 ) {
      duration += "0";
    }
    duration += std::to_string( t->tm_sec );

    writeAttribute( loc, "Duration", duration );
  }

  void Hdf5Writer::writeAttributes( H5::H5Object& ds, SignalData * data ) {
    // writeTimesAndDurationAttributes( ds, data.startTime( ), data.endTime( ) );
    for ( const auto& m : data->metad( ) ) {
      writeAttribute( ds, m.first, m.second );
    }
    for ( const auto& m : data->metas( ) ) {
      writeAttribute( ds, m.first, m.second );
    }
    for ( const auto& m : data->metai( ) ) {
      writeAttribute( ds, m.first, m.second );
    }
  }

  std::string Hdf5Writer::getDatasetName( SignalData * data ) {
    return getDatasetName( data->name( ) );
  }

  std::string Hdf5Writer::getDatasetName( const std::string& oldname ) {
    std::string name( oldname );
    std::map<std::string, std::string> replacements;
    replacements["%"] = "pct";
    replacements["("] = "_";
    replacements[")"] = "_";
    replacements["/"] = "_";
    replacements["\\"] = "_";
    replacements["-"] = "_";
    replacements["."] = "_";

    for ( auto&pr : replacements ) {
      size_t pos = name.find( pr.first );
      while ( std::string::npos != pos ) {
        // Replace this occurrence of Sub String
        name.replace( pos, pr.first.size( ), pr.second );
        // Get the next occurrence from the current position
        pos = name.find( pr.first, pos + pr.first.size( ) );
      }
    }

    return name;
  }

  void Hdf5Writer::writeFileAttributes( H5::H5File file,
      std::map<std::string, std::string> datasetattrs,
      const dr_time& firstTime, const dr_time& lastTime ) {

    writeTimesAndDurationAttributes( file, firstTime, lastTime );

    for ( auto& it : datasetattrs ) {
      Log::debug( ) << "writing file attr: " << it.first << ": " << it.second << std::endl;
      if ( OffsetTimeSignalSet::COLLECTION_OFFSET == it.first ) {
        writeAttribute( file, it.first, std::stoi( it.second ) );
      }
      else {
        writeAttribute( file, it.first, it.second );
      }
    }

    writeAttribute( file, "Layout Version", LAYOUT_VERSION );
    writeAttribute( file, "HDF5 Version",
        std::to_string( H5_VERS_MAJOR ) + "."
        + std::to_string( H5_VERS_MINOR ) + "."
        + std::to_string( H5_VERS_RELEASE ) );
  }

  void Hdf5Writer::writeVital( H5::Group& group, SignalData * data ) {
    // two sections of this function
    // 1: write attributes
    // 2: write data
    std::vector<std::string> extras = data->extras( );
    hsize_t sz = data->size( );

    hsize_t dims[] = { sz, 1 + extras.size( ) };
    H5::DataSpace space( 2, dims );
    H5::DSetCreatPropList props;
    if ( compression( ) > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      autochunk( dims, 2, sizeof (short ), chunkdims );
      props.setChunk( 2, chunkdims );
      props.setShuffle( );
      props.setDeflate( compression( ) );
    }

    bool useInts = false;
    if ( rescaleForShortsIfNeeded( data, useInts ) ) {
      Log::warn( ) << std::endl << " coercing out-of-range numbers (possible loss of precision)";
    }

    H5::DataSet ds = group.createDataSet( "data",
        ( useInts ? H5::PredType::STD_I32LE : H5::PredType::STD_I16LE ), space, props );
    writeAttributes( ds, data );

    std::string cols = "scaled value";
    for ( auto& x : extras ) {
      cols.append( "," ).append( x );
    }

    writeAttribute( ds, "Columns", cols );

    size_t exc = extras.size( );
    const int scale = data->scale( );

    const size_t rows = data->size( );
    const hsize_t maxslabcnt = std::min( rows, (size_t) ( 1024 * 256 ) );
    hsize_t offset[] = { 0, 0 };
    hsize_t count[] = { 0, exc + 1 };

    Log::trace( ) << "writing " << rows << " rows of data in slabs of " << maxslabcnt << std::endl;

    std::vector<short> sbuffer;
    std::vector<int> ibuffer;

    if ( useInts ) {
      ibuffer.reserve( maxslabcnt * ( exc + 1 ) );
    }
    else {
      sbuffer.reserve( maxslabcnt * ( exc + 1 ) );
    }

    // We're keeping a buffer to eventually write to the file. However, this
    // buffer is based on a number of rows, *not* some multiple of the data size.
    // This means we are really keeping two counters going at all times, and
    // once we're out of all the loops, we need to write any residual data.
    size_t rowcount = 0;
    for ( size_t row = 0; row < rows; row++ ) {
      auto datarow = data->pop( );
      datarow->rescale( scale );
      if ( useInts ) {
        ibuffer.push_back( datarow->ints( )[0] );
      }
      else {
        sbuffer.push_back( datarow->shorts( )[0] );
      }
      if ( !extras.empty( ) ) {
        for ( size_t i = 0; i < exc; i++ ) {
          short val = SignalData::MISSING_VALUE;
          const std::string xkey = extras.at( i );
          if ( 0 != datarow->extras.count( xkey ) ) {
            const auto& x = datarow->extras.at( xkey );
            val = static_cast<short> ( std::stoi( x ) );
          }
          if ( useInts ) {
            ibuffer.push_back( val );
          }
          else {
            sbuffer.push_back( val );
          }
        }
      }

      if ( ++rowcount >= maxslabcnt ) {
        offset[0] += count[0];
        count[0] = rowcount;
        space.selectHyperslab( H5S_SELECT_SET, count, offset );

        H5::DataSpace memspace( 2, count );
        if ( useInts ) {
          Log::trace( ) << "slab" << std::endl;
          ds.write( ibuffer.data(), H5::PredType::STD_I32LE, memspace, space );
          ibuffer.clear( );
          ibuffer.reserve( maxslabcnt * ( exc + 1 ) );
        }
        else {
          Log::trace( ) << "slab" << std::endl;
          ds.write( sbuffer.data(), H5::PredType::STD_I16LE, memspace, space );
          sbuffer.clear( );
          sbuffer.reserve( maxslabcnt * ( exc + 1 ) );
        }
        rowcount = 0;
      }
    }

    // finally, write whatever's left in the buffer
    if ( useInts ) {
      if ( !ibuffer.empty( ) ) {
        offset[0] += count[0];
        count[0] = rowcount;
        space.selectHyperslab( H5S_SELECT_SET, count, offset );
        H5::DataSpace memspace( 2, count );
        Log::trace( ) << "slab - leftovers" << std::endl;
        ds.write( ibuffer.data(), H5::PredType::STD_I32LE, memspace, space );
      }
    }
    else {
      if ( !sbuffer.empty( ) ) {
        offset[0] += count[0];
        count[0] = rowcount;
        space.selectHyperslab( H5S_SELECT_SET, count, offset );
        H5::DataSpace memspace( 2, count );
        Log::trace( ) << "slab - leftovers" << std::endl;
        ds.write( sbuffer.data(), H5::PredType::STD_I16LE, memspace, space );
      }
    }
  }

  void Hdf5Writer::writeWave( H5::Group& group, SignalData * data ) {
    const hsize_t rows = data->size( );
    const int scale = data->scale( );
    const int valsperrow = data->readingsPerChunk( );

    hsize_t dims[] = { rows * valsperrow, 1 };
    H5::DataSpace space( 2, dims );
    H5::DSetCreatPropList props;
    if ( compression( ) > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      autochunk( dims, 2, sizeof ( short ), chunkdims );
      props.setChunk( 2, chunkdims );
      props.setShuffle( );
      props.setDeflate( compression( ) );
    }

    bool useInts = false;
    if ( rescaleForShortsIfNeeded( data, useInts ) ) {
      Log::warn( ) << std::endl << "  coercing out-of-range numbers (possible loss of precision)";
    }

    H5::DataSet ds = group.createDataSet( "data",
        ( useInts ? H5::PredType::STD_I32LE : H5::PredType::STD_I16LE ), space, props );
    writeAttributes( ds, data );
    writeAttribute( ds, "Columns", "scaled value" );

    const hsize_t maxslabcnt = std::min( rows * valsperrow, (hsize_t) ( 1024 * 256 ) );
    hsize_t offset[] = { 0, 0 };
    hsize_t count[] = { 0, 1 };

    Log::trace( ) << "writing " << rows * valsperrow << " values in slabs of " << maxslabcnt << std::endl;

    std::vector<short> sbuffer;
    std::vector<int> ibuffer;
    if ( useInts ) {
      ibuffer.reserve( maxslabcnt );
    }
    else {
      sbuffer.reserve( maxslabcnt );
    }

    // We're keeping a buffer to eventually write to the file. However, this
    // buffer is based on a number of rows, *not* some multiple of the data size.
    // This means we are really keeping two counters going at all times, and
    // once we're out of all the loops, we need to write any residual data.

    for ( size_t row = 0; row < rows; row++ ) {
      std::unique_ptr<FormatConverter::DataRow> datarow = data->pop( );
      datarow->rescale( scale );
      if ( useInts ) {
        const auto& ints = datarow->ints( );
        ibuffer.insert( ibuffer.end( ), ints.begin( ), ints.end( ) );
      }
      else {
        std::vector<short> ints = datarow->shorts( );
        sbuffer.insert( sbuffer.end( ), ints.begin( ), ints.end( ) );
      }

      if ( useInts ) {
        if ( ibuffer.size( ) >= maxslabcnt ) {
          count[0] = ibuffer.size( );
          space.selectHyperslab( H5S_SELECT_SET, count, offset );
          offset[0] += count[0];

          H5::DataSpace memspace( 2, count );
          Log::trace( ) << "slab " << ( offset[0] - count[0] ) << std::endl;
          ds.write( ibuffer.data(), H5::PredType::STD_I32LE, memspace, space );
          ibuffer.clear( );
          ibuffer.reserve( maxslabcnt );
        }
      }
      else {
        if ( sbuffer.size( ) >= maxslabcnt ) {
          count[0] = sbuffer.size( );
          space.selectHyperslab( H5S_SELECT_SET, count, offset );
          offset[0] += count[0];

          H5::DataSpace memspace( 2, count );
          Log::trace( ) << "slab " << ( offset[0] - count[0] ) << std::endl;
          ds.write( sbuffer.data(), H5::PredType::STD_I16LE, memspace, space );
          sbuffer.clear( );
          sbuffer.reserve( maxslabcnt );
        }
      }
    }

    // finally, write whatever's left in the buffer
    if ( useInts ) {
      if ( !ibuffer.empty( ) ) {
        count[0] = ibuffer.size( );
        space.selectHyperslab( H5S_SELECT_SET, count, offset );
        H5::DataSpace memspace( 2, count );
        Log::trace( ) << "slab - leftovers" << std::endl;
        ds.write( ibuffer.data(), H5::PredType::STD_I32LE, memspace, space );
      }
    }
    else {
      if ( !sbuffer.empty( ) ) {
        count[0] = sbuffer.size( );
        space.selectHyperslab( H5S_SELECT_SET, count, offset );
        H5::DataSpace memspace( 2, count );
        Log::trace( ) << "slab - leftovers" << std::endl;
        ds.write( sbuffer.data(), H5::PredType::STD_I16LE, memspace, space );
      }
    }
  }

  void Hdf5Writer::autochunk( hsize_t* dims, int rank, int bytesperelement, hsize_t* rslts ) {
    // goal: keep chunksize betwee 16kb and 1M (2^4 to 2^10 kb)
    // with smaller chunks for smaller datasets

    const hsize_t KB = 1024;
    const hsize_t MB = 1024 * KB;

    // datasets up to this size are deemed to fall under the "regular" algorithm
    const hsize_t NORMAL_MAX_SIZE_LIMIT = 128 * MB;

    hsize_t dselements = 1;
    hsize_t rowelements = 1;
    for ( int i = 0; i < rank; i++ ) {
      rslts[i] = dims[i];
      dselements *= dims[i];

      if ( 0 != i ) {
        rowelements *= dims[i];
      }
    }
    const hsize_t datasetsize = dselements * bytesperelement;
    const hsize_t bytesperrow = rowelements * bytesperelement;

    if ( datasetsize <= NORMAL_MAX_SIZE_LIMIT ) {

      // NOTE: all these limits are basically arbitrary; we're just trying to
      // keep the chunk size "not too big, not too small", and less than 1M
      // (for the default HDF5 chunk cache)
      std::map<hsize_t, hsize_t> LIMITS;
      LIMITS[368 * KB] = datasetsize; // small datasets get everything in one chunk
      LIMITS[512 * KB] = 64 * KB;
      LIMITS[MB] = 128 * KB;
      LIMITS[4 * MB] = 256 * KB;
      LIMITS[8 * MB] = 512 * KB;
      LIMITS[12 * MB] = 768 * KB;
      LIMITS[NORMAL_MAX_SIZE_LIMIT] = MB;


      hsize_t chunksize = 0;
      for ( auto x : LIMITS ) {
        if ( datasetsize <= x.first ) {
          chunksize = x.second;
          break;
        }
      }

      // now convert our desired chunk size into the number of rows per chunk
      // NOTE: only changing the first dimension, so rows stay together
      rslts[0] = chunksize / bytesperrow;
      return;
    }

    // we have a dataset over 256MB, so only chunk every column
    rslts[0] = MB / bytesperelement; // per element, because every column is chunked
    for ( int i = 1; i < rank; i++ ) {
      rslts[i] = 1;
    }
  }

  void Hdf5Writer::createEventsAndTimes( H5::H5File file, const SignalSet * data ) {
    Log::trace( ) << "creating events and times" << std::endl;
    auto events = ensureGroupExists( file, "Events" );

    auto segmentsizes = data->offsets( );
    if ( !segmentsizes.empty( ) ) {
      hsize_t dims[] = { segmentsizes.size( ), 2 };
      H5::DataSpace space( 2, dims );

      auto ds = events.createDataSet( "Segment_Offsets", H5::PredType::STD_I64LE, space );
      auto indexes = std::vector<long long>( segmentsizes.size( ) * 2 );
      int row = 0;
      for ( const auto& e : segmentsizes ) {
        indexes[ 2 * row ] = e.second;
        indexes[2 * row + 1] = e.first;
        row++;
      }
      ds.write( indexes.data( ), H5::PredType::STD_I64LE );
      writeAttribute( ds, "Columns", "timestamp (ms), segment offset" );
    }

    // we want "sort of unique" global times. That is, if a time is duplicated
    // in a signal, we want duplicates in our time list. However, if two signals
    // have the same time, we don't want duplicates

    // our algorithm: iterate over all the times for all signals, and add the
    // lowest time to the dataset, then move the iterators with the lowest time
    Log::trace( ) << "sorting times" << std::endl;
    auto begins = std::vector<TimeRange::iterator>{ };
    auto ends = std::vector<TimeRange::iterator>{ };
    for ( auto m : data->allsignals( ) ) {
      // no sense in checking times on signals without data
      if ( !m->empty( ) ) {
        begins.push_back( timecache.at( m )->begin( ) );
        ends.push_back( timecache.at( m )->end( ) );
      }
    }

    auto globals = std::make_unique<TimeRange>( );
    auto empties = std::set<int>( );

    while ( empties.size( ) != begins.size( ) ) {
      auto lowest = std::numeric_limits<dr_time>::max( );

      for ( size_t i = 0; i < begins.size( ); i++ ) {
        if ( 0 == empties.count( i ) ) {
          //figure out what our lowest value is
          if ( *begins[i] < lowest ) {
            lowest = *begins[i];
          }
        }
      }

      // add the lowest value to the global time range
      globals->push_back( lowest );

      // go through the iterators again, advancing the ones that match the lowest value
      for ( size_t i = 0; i < begins.size( ); i++ ) {
        if ( 0 == empties.count( i ) ) {
          if ( *begins[i] == lowest ) {
            begins[i]++;
          }
          if ( begins[i] == ends[i] ) {
            empties.insert( i );
          }
        }
      }
    }

    // our globals time range should now have all times and the appropriate duplicates
    auto ds = writeTimes( events, globals.get( ), "Global_Times" );
    writeAttribute( ds, "Columns", "timestamp (ms)" );

    //for ( auto x : data->metadata( ) ) {
    //  std::cout << "hdf5writer: " << x.first << ": " << x.second << std::endl;
    //}
    writeAttribute( ds, SignalData::TIMEZONE, data->metadata( ).at( SignalData::TIMEZONE ) );

    if ( 0 != data->metadata( ).count( TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE ) ) {
      writeAttribute( ds, TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE, data->metadata( )
          .at( TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE ) );
    }
    if ( 0 != data->metadata( ).count( OffsetTimeSignalSet::COLLECTION_OFFSET ) ) {
      writeAttribute( ds, OffsetTimeSignalSet::COLLECTION_OFFSET, std::stoi( data->metadata( )
          .at( OffsetTimeSignalSet::COLLECTION_OFFSET ) ) );
    }

    // finally, if we're using indexed time, create the mapping
    if ( Options::asBool( FormatConverter::OptionsKey::INDEXED_TIME ) ) {
      for ( auto time : *globals ) {
        timesteplkp.insert( std::make_pair( time, timesteplkp.size( ) ) );
      }
    }
  }

  H5::Group Hdf5Writer::ensureGroupExists( H5::H5Object& obj, const std::string& s ) {
    return ( obj.exists( s )
        ? obj.openGroup( s )
        : obj.createGroup( s ) );
  }

  int Hdf5Writer::drain( SignalSet * info ) {
    dataptr = info;
    return 0;
  }

  void Hdf5Writer::createTimeCache( ) {
    timecache.clear( );
    for ( auto sig : dataptr->allsignals( ) ) {
      timecache[sig] = sig->times( );
    }
  }

  std::vector<std::string> Hdf5Writer::closeDataSet( ) {
    auto firstTime = dataptr->earliest( );
    auto lastTime = dataptr->latest( );
    std::vector<std::string> ret;
    if ( 0 == lastTime ) {
      // we don't have any data at all!
      return ret;
    }

    std::string outy = filenamer( ).filename( dataptr );

    if ( dataptr->vitals( ).empty( ) && dataptr->waves( ).empty( ) ) {
      Log::warn( ) << "Nothing to write to " << outy << std::endl;
      return ret;
    }

    Log::info( ) << "Writing to " << outy << std::endl;

    H5::Exception::dontPrint( );
    try {
      H5::H5File file( outy, H5F_ACC_TRUNC );

      createTimeCache( );
      createEventsAndTimes( file, dataptr ); // also creates the timesteplkp

      auto auxdata = dataptr->auxdata( );
      if ( !auxdata.empty( ) ) {
        for ( auto& fileaux : auxdata ) {
          writeAuxData( file, fileaux.first, fileaux.second );
        }
      }

      writeFileAttributes( file, dataptr->metadata( ), firstTime, lastTime );

      auto grp = ensureGroupExists( file, "VitalSigns" );
      Log::debug( ) << "Writing " << dataptr->vitals( ).size( ) << " Vitals" << std::endl;
      for ( auto& vits : dataptr->vitals( ) ) {
        if ( vits->empty( ) ) {
          Log::warn( ) << "Skipping Vital: " << vits->name( ) << "(no data)" << std::endl;
        }
        else {
          auto g = ensureGroupExists( grp, getDatasetName( vits ) );
          writeVitalGroup( g, vits );
        }
      }

      if ( !this->skipwaves( ) ) {
        grp = ensureGroupExists( file, "Waveforms" );
        Log::debug( ) << "Writing " << dataptr->waves( ).size( ) << " Waveforms" << std::endl;
        for ( auto& wavs : dataptr->waves( ) ) {
          if ( wavs->empty( ) ) {
            Log::warn( ) << "Skipping Wave: " << wavs->name( ) << "(no data)" << std::endl;
          }
          else {
            H5::Group g = ensureGroupExists( grp, getDatasetName( wavs ) );
            writeWaveGroup( g, wavs );
          }
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

  void Hdf5Writer::drain( H5::Group& g, SignalData * data ) {
    if ( data->wave( ) ) {
      writeWaveGroup( g, data );
    }
    else {
      writeVitalGroup( g, data );
    }
  }

  void Hdf5Writer::writeGroupAttrs( H5::Group& group, SignalData * data ) {
    writeTimesAndDurationAttributes( group, data->startTime( ), data->endTime( ) );
    writeAttribute( group, SignalData::LABEL, data->name( ) );
    writeAttribute( group, SignalData::TIMEZONE, data->metas( ).at( SignalData::TIMEZONE ) );
    if ( 0 != data->metai( ).count( SignalData::CHUNK_INTERVAL_MS ) ) {
      writeAttribute( group, SignalData::CHUNK_INTERVAL_MS, data->metai( ).at( SignalData::CHUNK_INTERVAL_MS ) );
    }
    if ( 0 != data->metai( ).count( SignalData::READINGS_PER_CHUNK ) ) {
      writeAttribute( group, SignalData::READINGS_PER_CHUNK, data->metai( ).at( SignalData::READINGS_PER_CHUNK ) );
    }
    writeAttribute( group, SignalData::UOM, data->uom( ) );
  }

  void Hdf5Writer::writeVitalGroup( H5::Group& group, SignalData * data ) {
    Log::debug( ) << "Writing Vital: " << data->name( ) << std::endl;

    writeGroupAttrs( group, data );
    writeTimes( group, data );
    writeVital( group, data );
    writeEvents( group, data );
    for ( const auto& aux : data->auxdata( ) ) {
      writeAuxData( group, aux.first, aux.second );
    }
  }

  bool Hdf5Writer::rescaleForShortsIfNeeded( SignalData * data, bool& useIntsNotShorts ) const {
    useIntsNotShorts = false;

    const auto SHORTMAX = std::numeric_limits<short>::max( );
    const auto SHORTMIN = std::numeric_limits<short>::min( );

    int powscale = std::pow( 10, data->scale( ) );
    auto hi = powscale * data->highwater( );
    auto low = powscale * data->lowwater( );

    if ( hi < SHORTMAX && low > SHORTMIN ) {
      useIntsNotShorts = false;
      return false;
    }
    else {
      useIntsNotShorts = true;
      return false;
    }
  }

  void Hdf5Writer::writeWaveGroup( H5::Group& group, SignalData * data ) {

    Log::debug( ) << "Writing Wave: " << data->name( );
    auto st = std::chrono::high_resolution_clock::now( );

    writeTimes( group, data );
    writeWave( group, data );
    writeEvents( group, data );
    writeGroupAttrs( group, data );
    for ( const auto& aux : data->auxdata( ) ) {
      writeAuxData( group, aux.first, aux.second );
    }

    auto en = std::chrono::high_resolution_clock::now( );
    std::chrono::duration<float> dur = en - st;
    Log::trace( ) << " (complete in " << dur.count( ) << "s)";
    Log::debug( ) << std::endl;
  }

  H5::DataSet Hdf5Writer::writeTimes( H5::Group& group, TimeRange * times, const std::string& dsname ) {
    const auto ROWS = times->size( );

    const auto SLABSIZE = std::min( ROWS, TimeRange::DEFAULT_CACHE_LIMIT );

    Log::trace( ) << "writing " << ROWS << " times in slabs of " << SLABSIZE << std::endl;

    hsize_t dims[] = { ROWS, 1 };
    H5::DataSpace space( 2, dims );

    H5::DSetCreatPropList props;
    if ( compression( ) > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      autochunk( dims, 2, sizeof (long ), chunkdims );
      props.setChunk( 2, chunkdims );
      props.setShuffle( );
      props.setDeflate( compression( ) );
    }

    H5::DataSet ds = group.createDataSet( dsname, H5::PredType::STD_I64LE, space, props );
    hsize_t offset[] = { 0, 0 };
    hsize_t count[] = { 0, 1 };

    size_t startidx = 0;
    auto buffer = std::vector<dr_time>( );
    while ( startidx < ROWS ) {
      const auto ADDS = std::min( SLABSIZE, ROWS - startidx );
      auto endidx = startidx + ADDS;

      offset[0] += count[0];
      count[0] = ADDS;
      space.selectHyperslab( H5S_SELECT_SET, count, offset );

      H5::DataSpace memspace( 2, count );
      times->fill( buffer, startidx, endidx );

      Log::trace( ) << "slab - " << ADDS << " rows" << std::endl;
      ds.write( buffer.data( ), H5::PredType::STD_I64LE, memspace, space );
      buffer.clear( );
      startidx += ADDS;
    }

    return ds;
  }

  void Hdf5Writer::writeTimes( H5::Group& group, SignalData * data ) {
    auto& times = timecache.at( data );
    H5::DataSet ds;
    if ( FormatConverter::Options::asBool( FormatConverter::OptionsKey::INDEXED_TIME ) ) {
      // convert dr_times to the index of the global times array
      auto timeidxs = std::make_unique<TimeRange>( );
      for ( auto it : *times ) {
        timeidxs->push_back( timesteplkp.at( it ) );
      }
      ds = writeTimes( group, timeidxs.get( ) );
    }
    else {
      ds = writeTimes( group, times.get( ) );
    }

    writeAttribute( ds, "Time Source", "raw" );
    writeAttribute( ds, "Columns", FormatConverter::Options::asBool( FormatConverter::OptionsKey::INDEXED_TIME )
        ? "index to Global_Times"
        : "timestamp (ms)" );

    writeAttribute( ds, SignalData::TIMEZONE, data->metas( ).at( SignalData::TIMEZONE ) );
    // writeAttribute( ds, SignalData::VALS_PER_DR, data.valuesPerDataRow( ) );
    if ( 0 != data->metas( ).count( TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE ) ) {
      writeAttribute( ds, TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE, data->metas( )
          .at( TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE ) );
    }
    if ( 0 != data->metai( ).count( OffsetTimeSignalSet::COLLECTION_OFFSET ) ) {

      writeAttribute( ds, OffsetTimeSignalSet::COLLECTION_OFFSET,
          data->metai( ).at( OffsetTimeSignalSet::COLLECTION_OFFSET ) );
    }
  }

  void Hdf5Writer::writeAuxData( H5::Group& group, const std::string& name,
      const std::vector<TimedData>& data ) {
    if ( data.empty( ) ) {
      return;
    }

    const size_t sz = data.size( );
    H5::Group parent = ensureGroupExists( group, "Auxillary_Data" );
    H5::Group auxg = ensureGroupExists( parent, getDatasetName( name ) );

    auto times = std::make_unique<TimeRange>( );

    // HDF5 seems to have trouble writing a vector of strings, so
    // we'll just load the data into an array of char *s.
    auto vdata = std::vector<const char*>( sz );

    size_t c = 0;
    for ( const auto& t : data ) {
      times->push_back( t.time );
      vdata[c++] = t.data.c_str( );
    }

    writeTimes( auxg, times.get( ) );

    hsize_t dims[] = { sz, 1 };
    H5::DataSpace space( 2, dims );

    H5::StrType st( H5::PredType::C_S1, H5T_VARIABLE );
    st.setCset( H5T_CSET_UTF8 );

    H5::DSetCreatPropList props;
    if ( compression( ) > 0 ) {
      hsize_t chunkdims[] = { sz, 1 };
      props.setChunk( 2, chunkdims );
      props.setShuffle( );
      props.setDeflate( compression( ) );
    }

    H5::DataSet dsv = auxg.createDataSet( "data", st, space, props );
    Log::trace( ) << "writing auxillary data" << std::endl;
    dsv.write( vdata.data( ), st );
  }

  void Hdf5Writer::writeEvents( H5::Group& group, SignalData * data ) {
    std::vector<std::string> types = data->eventtypes( );
    if ( types.empty( ) ) {
      return;
    }

    Log::trace( ) << "writing events" << std::endl;

    H5::Group eventgroup = ensureGroupExists( group, "events" );
    for ( auto& type : types ) {
      auto times = data->events( type );

      hsize_t dims[] = { times.size( ), 1 };
      H5::DataSpace space( 2, dims );

      H5::DSetCreatPropList props;
      if ( compression( ) > 0 ) {
        hsize_t chunkdims[] = { 0, 0 };
        autochunk( dims, 2, sizeof (long ), chunkdims );
        props.setChunk( 2, chunkdims );
        props.setShuffle( );
        props.setDeflate( compression( ) );
      }

      H5::DataSet ds = eventgroup.createDataSet( getDatasetName( type ),
          H5::PredType::STD_I64LE, space, props );
      ds.write( times.data(), H5::PredType::STD_I64LE );
      writeAttribute( ds, SignalData::LABEL, type );
      writeAttribute( ds, "Time Source", "raw" );
      writeAttribute( ds, "Columns", "timestamp (ms)" );
      writeAttribute( ds, SignalData::TIMEZONE, data->metas( ).at( SignalData::TIMEZONE ) );
      // writeAttribute( ds, SignalData::VALS_PER_DR, data.valuesPerDataRow( ) );
      if ( 0 != data->metas( ).count( TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE ) ) {
        writeAttribute( ds, TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE, data->metas( )
            .at( TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE ) );
      }
      if ( 0 != data->metai( ).count( OffsetTimeSignalSet::COLLECTION_OFFSET ) ) {
        writeAttribute( ds, OffsetTimeSignalSet::COLLECTION_OFFSET,
            data->metai( ).at( OffsetTimeSignalSet::COLLECTION_OFFSET ) );
      }
    }
  }
}