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
#include "Hdf5Writer.h"
#include "SignalData.h"
#include "DataRow.h"
#include "SignalUtils.h"
#include "FileNamer.h"
#include "H5public.h"
#include "OffsetTimeSignalSet.h"

const std::string Hdf5Writer::LAYOUT_VERSION = "4.0.0";

Hdf5Writer::Hdf5Writer( ) : Writer( "hdf5" ) {
}

Hdf5Writer::Hdf5Writer( const Hdf5Writer& orig ) : Writer( "hdf5" ) {
}

Hdf5Writer::~Hdf5Writer( ) {
}

void Hdf5Writer::writeAttribute( H5::H5Location& loc,
    const std::string& attr, const std::string& val ) {
  if ( !val.empty( ) ) {
    //std::cout << attr << ": " << val << std::endl;

    H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
    H5::StrType st( H5::PredType::C_S1, H5T_VARIABLE );
    st.setCset( H5T_CSET_UTF8 );
    H5::Attribute attrib = loc.createAttribute( attr, st, space );
    attrib.write( st, val );
  }
}

void Hdf5Writer::writeAttribute( H5::H5Location& loc,
    const std::string& attr, int val ) {
  //std::cout << "writing attribute (int):" << attr << ": "<<val<<std::endl;
  H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
  H5::Attribute attrib = loc.createAttribute( attr, H5::PredType::STD_I32LE, space );
  attrib.write( H5::PredType::STD_I32LE, &val );
}

void Hdf5Writer::writeAttribute( H5::H5Location& loc,
    const std::string& attr, dr_time val ) {
  H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
  H5::Attribute attrib = loc.createAttribute( attr, H5::PredType::STD_I64LE, space );
  attrib.write( H5::PredType::STD_I64LE, &val );
}

void Hdf5Writer::writeAttribute( H5::H5Location& loc,
    const std::string& attr, double val ) {
  H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
  H5::Attribute attrib = loc.createAttribute( attr, H5::PredType::IEEE_F64LE, space );
  attrib.write( H5::PredType::IEEE_F64LE, &val );
}

void Hdf5Writer::writeTimesAndDurationAttributes( H5::H5Location& loc,
    const dr_time& start, const dr_time& end ) {

  time_t stime = start / 1000;
  time_t etime = end / 1000;
  writeAttribute( loc, "Start Time", start );
  writeAttribute( loc, "End Time", end );

  char buf[sizeof "2011-10-08T07:07:09Z"];
  strftime( buf, sizeof buf, "%FT%TZ", gmtime( &stime ) );

  writeAttribute( loc, "Start Date/Time", buf );
  buf[sizeof "2011-10-08T07:07:09Z"];
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

void Hdf5Writer::writeAttributes( H5::H5Location& ds, const std::unique_ptr<SignalData>& data ) {
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

std::string Hdf5Writer::getDatasetName( const std::unique_ptr<SignalData>& data ) {
  std::string name( data->name( ) );
  std::map<std::string, std::string> replacements;
  replacements["%"] = "perc";
  replacements["("] = "_";
  replacements[")"] = "_";
  replacements["/"] = "_";

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

  for ( std::map<std::string, std::string>::const_iterator it = datasetattrs.begin( );
      it != datasetattrs.end( ); ++it ) {
    //std::cout << "writing file attr: " << it->first << ": " << it->second << std::endl;
    if ( it->first == OffsetTimeSignalSet::COLLECTION_TIMEZONE_OFFSET ) {
      writeAttribute( file, it->first, std::stoi( it->second ) );
    }
    else {
      writeAttribute( file, it->first, it->second );
    }
  }

  writeAttribute( file, "Layout Version", LAYOUT_VERSION );
  writeAttribute( file, "HDF5 Version",
      std::to_string( H5_VERS_MAJOR ) + "."
      + std::to_string( H5_VERS_MINOR ) + "."
      + std::to_string( H5_VERS_RELEASE ) );
}

void Hdf5Writer::writeVital( H5::Group& group, std::unique_ptr<SignalData>& data ) {
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
    autochunk( dims, 2, chunkdims );
    props.setChunk( 2, chunkdims );
    props.setDeflate( compression( ) );
  }

  if ( rescaleForShortsIfNeeded( data ) ) {
    std::cerr << std::endl << " coercing out-of-range numbers (possible loss of precision)";
  }

  H5::DataSet ds = group.createDataSet( "data", H5::PredType::STD_I16LE, space, props );
  writeAttributes( ds, data );

  std::string cols = "scaled value";
  for ( auto& x : extras ) {
    cols.append( "," ).append( x );
  }

  writeAttribute( ds, "Columns", cols );

  const int rows = data->size( );
  const size_t exc = extras.size( );
  short buffer[rows * ( exc + 1 )] = { 0 };
  int scale = data->scale( );

  for ( int row = 0; row < rows; row++ ) {
    const std::unique_ptr<DataRow>& datarow = data->pop( );

    long baseidx = ( exc + 1 ) * row;
    // FIXME: we don't want to scale missing values
    buffer[baseidx] = datarow->shorts( scale )[0];

    if ( !extras.empty( ) ) {
      for ( int i = 0; i < exc; i++ ) {
        short val = SignalData::MISSING_VALUE;
        const std::string xkey = extras.at( i );
        if ( 0 != datarow->extras.count( xkey ) ) {
          const auto& x = datarow->extras.at( xkey );
          val = (short) ( std::stoi( x ) );
        }
        buffer[baseidx + ( i + 1 )] = val;
      }
    }
  }

  ds.write( &buffer, H5::PredType::STD_I16LE );
}

void Hdf5Writer::writeWave( H5::Group& group, std::unique_ptr<SignalData>& data ) {
  hsize_t sz = data->size( );

  int valsperrow = data->readingsPerSample( );

  hsize_t dims[] = { sz * valsperrow, 1 };
  H5::DataSpace space( 2, dims );
  H5::DSetCreatPropList props;
  if ( compression( ) > 0 ) {
    hsize_t chunkdims[] = { 0, 0 };
    autochunk( dims, 2, chunkdims );
    props.setChunk( 2, chunkdims );
    props.setDeflate( compression( ) );
  }

  if ( rescaleForShortsIfNeeded( data ) ) {
    std::cerr << std::endl << "  coercing out-of-range numbers (possible loss of precision)";
  }

  H5::DataSet ds = group.createDataSet( "data", H5::PredType::STD_I16LE, space, props );
  writeAttributes( ds, data );
  writeAttribute( ds, "Columns", "scaled value" );


  const size_t rows = data->size( );
  const hsize_t maxslabcnt = ( rows * data->readingsPerSample( ) > 125000
      ? 125000
      : rows * data->readingsPerSample( ) );
  hsize_t offset[] = { 0, 0 };
  hsize_t count[] = { 0, 1 };

  std::vector<short> buffer;
  buffer.reserve( maxslabcnt );

  const int scale = std::pow( 10, data->scale( ) );

  // We're keeping a buffer to eventually write to the file. However, this 
  // buffer is based on a number of rows, *not* some multiple of the data size.
  // This means we are really keeping two counters going at all times, and 
  // once we're out of all the loops, we need to write any residual data.

  for ( int row = 0; row < rows; row++ ) {
    std::unique_ptr<DataRow> datarow = data->pop( );
    std::vector<short> ints = datarow->shorts( scale );
    buffer.insert( buffer.end( ), ints.begin( ), ints.end( ) );

    if ( buffer.size( ) >= maxslabcnt ) {
      count[0] = buffer.size( );
      space.selectHyperslab( H5S_SELECT_SET, count, offset );
      offset[0] += count[0];

      H5::DataSpace memspace( 2, count );
      ds.write( &buffer[0], H5::PredType::STD_I16LE, memspace, space );
      buffer.clear( );
      buffer.reserve( maxslabcnt );
    }
  }

  // finally, write whatever's left in the buffer
  if ( !buffer.empty( ) ) {
    count[0] = buffer.size( );
    space.selectHyperslab( H5S_SELECT_SET, count, offset );
    H5::DataSpace memspace( 2, count );
    ds.write( &buffer[0], H5::PredType::STD_I16LE, memspace, space );
  }
}

void Hdf5Writer::autochunk( hsize_t* dims, int rank, hsize_t* rslts ) {
  for ( int i = 0; i < rank; i++ ) {
    rslts[i] = dims[i];
  }

  if ( dims[0] < 1000 ) {
    return;
  }

  int dimscale = 1;
  if ( dims[0] > 32768 ) {
    dimscale = 2;
  }
  if ( dims[0] > 657536 ) {
    dimscale = 32;
  }
  if ( dims[0] > 1048576 ) {
    dimscale = 128;
  }
  if ( dims[0] > 16777216 ) {
    dimscale = 256;
  }

  rslts[0] = dims[0] / ( dims[1] * dimscale );
  rslts[1] = ( rslts[0] < 1000 ? 2 : 1 );
}

void Hdf5Writer::createEventsAndTimes( H5::H5File file, const std::unique_ptr<SignalSet>& data ) {
  H5::Group events = file.createGroup( "Events" );

  std::map<long, dr_time> segmentsizes = data->offsets( );
  if ( !segmentsizes.empty( ) ) {
    hsize_t dims[] = { segmentsizes.size( ), 2 };
    H5::DataSpace space( 2, dims );

    H5::DataSet ds = events.createDataSet( "Segment_Offsets",
        H5::PredType::STD_I64LE, space );
    long long indexes[segmentsizes.size( ) * 2] = { 0 };
    int row = 0;
    for ( const auto& e : segmentsizes ) {
      indexes[ 2 * row ] = e.second;
      indexes[2 * row + 1] = e.first;
      row++;
    }
    ds.write( indexes, H5::PredType::STD_I64LE );
    writeAttribute( ds, "Columns", "timestamp (ms), segment offset" );
  }

  // we want "sort of unique" global times. That is, if a time is duplicated
  // in a signal, we want duplicates in our time list. However, if two signals
  // have the same time, we don't want duplicates

  // our algorithm: keep a running count of each signal's time occurences.
  // after each signal, merge them with our "true" timecounter. Basically, if
  // timecounter and signalcounter agree, then don't change timecounter.
  // if signalcounter is greater than timecounter, update timecounter
  std::map<dr_time, int> timecounter;
  for ( const std::unique_ptr<SignalData>& m : data->allsignals( ) ) {
    std::map<dr_time, int> sigcounter;

    for ( auto& dr : m->times( ) ) {
      sigcounter[dr] = ( 0 == sigcounter.count( dr ) ? 1 : sigcounter[dr] + 1 );
    }

    for ( auto& it : sigcounter ) {
      if ( 0 == timecounter.count( it.first ) ) {
        timecounter[it.first] = it.second;
      }
      else if ( timecounter[it.first] < sigcounter[it.first] ) {
        timecounter[it.first] = sigcounter[it.first];
      }
    }
  }

  // convert our map to a vector with the appropriate duplicates
  std::vector<dr_time> alltimes;
  alltimes.reserve( timecounter.size( ) );

  for ( auto it = timecounter.begin( ); it != timecounter.end( ); it++ ) {
    for ( int i = 0; i < it->second; i++ ) {
      alltimes.push_back( it->first );
    }
  }

  // now we can make our dataset
  hsize_t dims[] = { alltimes.size( ), 1 };
  H5::DataSpace space( 2, dims );

  H5::DataSet ds = events.createDataSet( "Global_Times",
      H5::PredType::STD_I64LE, space );
  ds.write( &alltimes[0], H5::PredType::STD_I64LE );
  writeAttribute( ds, "Columns", "timestamp (ms)" );

  //for ( auto x : data->metadata( ) ) {
  //  std::cout << "hdf5writer: " << x.first << ": " << x.second << std::endl;
  //}
  writeAttribute( ds, SignalData::TIMEZONE, data->metadata( ).at( SignalData::TIMEZONE ) );

  if ( 0 != data->metadata( ).count( OffsetTimeSignalSet::COLLECTION_TIMEZONE ) ) {
    writeAttribute( ds, OffsetTimeSignalSet::COLLECTION_TIMEZONE, data->metadata( )
        .at( OffsetTimeSignalSet::COLLECTION_TIMEZONE ) );
  }
  if ( 0 != data->metadata( ).count( OffsetTimeSignalSet::COLLECTION_TIMEZONE_OFFSET ) ) {
    writeAttribute( ds, OffsetTimeSignalSet::COLLECTION_TIMEZONE_OFFSET, std::stoi( data->metadata( )
        .at( OffsetTimeSignalSet::COLLECTION_TIMEZONE_OFFSET ) ) );
  }
}

int Hdf5Writer::drain( std::unique_ptr<SignalSet>& info ) {
  // WARNING: we're stripping the pointer of it's uniqueness here
  dataptr = info.get( );
  return 0;
}

std::vector<std::string> Hdf5Writer::closeDataSet( ) {
  // UH OH: we're making another unique_ptr to this dataptr,
  // which is itself already owned by some other unique_ptr.
  // we MUST release it prior to ending this function
  std::unique_ptr<SignalSet> data( dataptr );

  dr_time firstTime = data->earliest( );
  dr_time lastTime = data->latest( );
  std::vector<std::string> ret;
  if ( 0 == lastTime ) {
    data.release( );
    // we don't have any data at all!
    return ret;
  }

  std::string outy = filenamer( ).filename( data );

  if ( data->vitals( ).empty( ) && data->waves( ).empty( ) ) {
    std::cerr << "Nothing to write to " << outy << std::endl;
    data.release( );
    return ret;
  }

  output( ) << "Writing to " << outy << std::endl;

  H5::Exception::dontPrint( );
  try {
    H5::H5File file( outy, H5F_ACC_TRUNC );
    writeFileAttributes( file, data->metadata( ), firstTime, lastTime );

    createEventsAndTimes( file, data );

    H5::Group grp = file.createGroup( "VitalSigns" );
    output( ) << "Writing " << data->vitals( ).size( ) << " Vitals" << std::endl;
    for ( auto& vits : data->vitals( ) ) {
      if ( vits.get( )->empty( ) ) {
        output( ) << "Skipping Vital: " << vits->name( ) << "(no data)" << std::endl;
      }
      else {
        H5::Group g = grp.createGroup( getDatasetName( vits ) );
        writeVitalGroup( g, vits );
      }
    }

    grp = file.createGroup( "Waveforms" );
    output( ) << "Writing " << data->waves( ).size( ) << " Waveforms" << std::endl;
    for ( auto& wavs : data->waves( ) ) {
      if ( wavs->empty( ) ) {
        output( ) << "Skipping Wave: " << wavs->name( ) << "(no data)" << std::endl;
      }
      else {
        H5::Group g = grp.createGroup( getDatasetName( wavs ) );
        writeWaveGroup( g, wavs );
      }
    }

    ret.push_back( outy );
  }
  // catch failure caused by the H5File operations
  catch ( H5::FileIException error ) {
    error.printError( );
  }
  // catch failure caused by the DataSet operations
  catch ( H5::DataSetIException error ) {
    error.printError( );
  }
  // catch failure caused by the DataSpace operations
  catch ( H5::DataSpaceIException error ) {
    error.printError( );
  }
  data.release( );
}

void Hdf5Writer::writeGroupAttrs( H5::Group& group, std::unique_ptr<SignalData>& data ) {
  writeTimesAndDurationAttributes( group, data->startTime( ), data->endTime( ) );
  writeAttribute( group, SignalData::LABEL, data->name( ) );
  writeAttribute( group, SignalData::TIMEZONE, data->metas( ).at( SignalData::TIMEZONE ) );
  writeAttribute( group, SignalData::CHUNK_INTERVAL_MS, data->metai( ).at( SignalData::CHUNK_INTERVAL_MS ) );
  writeAttribute( group, SignalData::READINGS_PER_CHUNK, data->metai( ).at( SignalData::READINGS_PER_CHUNK ) );
  writeAttribute( group, SignalData::UOM, data->uom( ) );

  data->erases( SignalData::TIMEZONE );
  data->erased( SignalData::CHUNK_INTERVAL_MS );
  data->erased( SignalData::READINGS_PER_CHUNK );
  data->erases( SignalData::UOM );
}

void Hdf5Writer::writeVitalGroup( H5::Group& group, std::unique_ptr<SignalData>& data ) {
  output( ) << "Writing Vital: " << data->name( ) << std::endl;

  writeGroupAttrs( group, data );
  writeTimes( group, data );
  writeVital( group, data );
}

bool Hdf5Writer::rescaleForShortsIfNeeded( std::unique_ptr<SignalData>& data ) const {
  bool rescaled = false;

  const short int max = std::numeric_limits<short>::max( );
  const short int min = std::numeric_limits<short>::min( );
  int scale = std::pow( 10, data->scale( ) );
  int hi = scale * data->highwater( );
  int low = ( data->lowwater( ) == SignalData::MISSING_VALUE
      ? data->lowwater( )
      : scale * data->lowwater( ) );
  //std::cerr << " high/low water marks: " << data.highwater( ) << "/" << data.lowwater( ) << "(scale: " << data.scale( ) << ")" << std::endl;
  //std::cerr << " high/low calcs: " << hi << "/" << low << "(scale: " << scale << ")" << std::endl;

  // keep reducing the scale until we fit in shorts
  // FIXME: if we can't fit in shorts, we're screwed
  while ( ( hi > max || low < min ) && scale > 0 ) {
    scale--;
    hi = std::pow( 10, scale ) * data->highwater( );
    low = ( data->lowwater( ) == SignalData::MISSING_VALUE
        ? data->lowwater( )
        : scale * data->lowwater( ) );
    rescaled = true;
    //std::cerr << " high/low calcs: " << hi << "/" << low << "(scale: " << scale << ")" << std::endl;
  }

  if ( hi > max || low < min ) {
    // this is "we're screwed" time
    std::cerr << " ERROR: cannot coerce values to be in range";
  }
  else {
    if ( rescaled ) {
      data->scale( scale );
    }
  }

  return rescaled;
}

void Hdf5Writer::writeWaveGroup( H5::Group& group, std::unique_ptr<SignalData>& data ) {
  output( ) << "Writing Wave: " << data->name( );
  auto st = std::chrono::high_resolution_clock::now( );

  writeTimes( group, data );
  writeWave( group, data );
  writeGroupAttrs( group, data );

  auto en = std::chrono::high_resolution_clock::now( );
  std::chrono::duration<float> dur = en - st;
  output( ) << " (complete in " << dur.count( ) << "s)" << std::endl;
}

void Hdf5Writer::writeTimes( H5::Group& group, std::unique_ptr<SignalData>& data ) {
  std::deque<dr_time> vec = data->times( );
  std::vector<dr_time> times( vec.rbegin( ), vec.rend( ) );

  hsize_t dims[] = { times.size( ), 1 };
  H5::DataSpace space( 2, dims );

  H5::DataSet ds = group.createDataSet( "time", H5::PredType::STD_I64LE, space );
  ds.write( &times[0], H5::PredType::STD_I64LE );
  writeAttribute( ds, "Time Source", "raw" );
  writeAttribute( ds, "Columns", "timestamp (ms)" );
  writeAttribute( ds, SignalData::TIMEZONE, data->metas( ).at( SignalData::TIMEZONE ) );
  // writeAttribute( ds, SignalData::VALS_PER_DR, data.valuesPerDataRow( ) );
}
