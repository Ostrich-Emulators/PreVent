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

#include "config.h"
#include "Hdf5Writer.h"
#include "SignalData.h"
#include "DataRow.h"
#include "SignalUtils.h"

Hdf5Writer::Hdf5Writer( ) {
}

Hdf5Writer::Hdf5Writer( const Hdf5Writer& orig ) {
}

Hdf5Writer::~Hdf5Writer( ) {
}

void Hdf5Writer::writeAttribute( H5::H5Location& loc,
    const std::string& attr, const std::string& val ) {
  if ( !val.empty( ) ) {
    H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
    H5::StrType st( H5::PredType::C_S1, H5T_VARIABLE );
    st.setCset( H5T_CSET_UTF8 );
    H5::Attribute attrib = loc.createAttribute( attr, st, space );
    attrib.write( st, val );
  }
}

void Hdf5Writer::writeAttribute( H5::H5Location& loc,
    const std::string& attr, int val ) {
  H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
  H5::Attribute attrib = loc.createAttribute( attr, H5::PredType::STD_I32LE, space );
  attrib.write( H5::PredType::STD_I32LE, &val );
}

void Hdf5Writer::writeAttribute( H5::H5Location& loc,
    const std::string& attr, double val ) {
  H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
  H5::Attribute attrib = loc.createAttribute( attr, H5::PredType::IEEE_F64LE, space );
  attrib.write( H5::PredType::IEEE_F64LE, &val );
}

void Hdf5Writer::writeTimesAndDurationAttributes( H5::H5Location& loc,
    const time_t& start, const time_t& end ) {
  writeAttribute( loc, "Start Time", (int) start );
  writeAttribute( loc, "End Time", (int) end );

  char buf[sizeof "2011-10-08T07:07:09Z"];
  strftime( buf, sizeof buf, "%FT%TZ", gmtime( &start ) );

  writeAttribute( loc, "Start Date/Time", buf );
  buf[sizeof "2011-10-08T07:07:09Z"];
  strftime( buf, sizeof buf, "%FT%TZ", gmtime( &end ) );
  writeAttribute( loc, "End Date/Time", buf );

  time_t xx( end - start );
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

void Hdf5Writer::writeAttributes( H5::DataSet& ds, const SignalData& data ) {

  writeTimesAndDurationAttributes( ds, data.startTime( ), data.endTime( ) );

  for ( const auto& m : data.metad( ) ) {
    writeAttribute( ds, m.first, m.second );
  }
  for ( const auto& m : data.metas( ) ) {
    writeAttribute( ds, m.first, m.second );
  }
  for ( const auto& m : data.metai( ) ) {
    writeAttribute( ds, m.first, m.second );
  }

  //  writeAttribute( ds, "Sample Period", period );
  //  writeAttribute( ds, "Sample Frequency", freq );
  //  writeAttribute( ds, "Sample Period Unit", "second" );
  //  writeAttribute( ds, "Sample Frequency Unit", "Hz" );
  //  writeAttribute( ds, "Unit of Measure", data.uom( ) );
  //  writeAttribute( ds, "Missing Value Marker", SignalData::MISSING_VALUE );
}

void Hdf5Writer::writeFileAttributes( H5::H5File file,
    std::map<std::string, std::string> datasetattrs,
    const time_t& firstTime, const time_t& lastTime ) {

  writeTimesAndDurationAttributes( file, firstTime, lastTime );

  for ( std::map<std::string, std::string>::const_iterator it = datasetattrs.begin( );
      it != datasetattrs.end( ); ++it ) {
    writeAttribute( file, it->first, it->second );
  }
}

void Hdf5Writer::writeVital( H5::DataSet& ds, H5::DataSpace&, SignalData& data ) {
  const int rows = data.size( );
  std::vector<std::string> extras = data.extras( );
  const size_t exc = extras.size( );
  short buffer[rows * ( exc + 1 )] = { 0 };
  int scale = data.scale( );
  for ( int row = 0; row < rows; row++ ) {
    const std::unique_ptr<DataRow>& datarow = data.pop( );

    long baseidx = ( exc + 1 ) * row;
    buffer[baseidx] = (short) ( std::stof( datarow->data ) * scale );

    if ( !extras.empty( ) ) {
      for ( int i = 0; i < exc; i++ ) {
        const auto& x = datarow->extras.at( extras.at( i ) );
        buffer[baseidx + ( i + 1 )] = (short) ( std::stof( x ) );
      }
    }
  }

  ds.write( &buffer, H5::PredType::STD_I16LE );
}

void Hdf5Writer::writeWave( H5::DataSet& ds, H5::DataSpace& space,
    SignalData& data ) {

  const size_t rows = data.size( );
  const hsize_t maxslabcnt = 125000;
  hsize_t offset[] = { 0, 0 };
  hsize_t count[] = { 0, 1 };

  std::vector<short> buffer;
  buffer.reserve( maxslabcnt );

  // We're keeping a buffer to eventually write to the file. However, this 
  // buffer is based on a number of rows, *not* some multiple of the data size.
  // This means we are really keeping two counters going at all times, and 
  // once we're out of all the loops, we need to write any residual data.

  for ( int row = 0; row < rows; row++ ) {
    std::unique_ptr<DataRow> datarow = data.pop( );
    std::vector<short> ints = datarow->shorts( );
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

void Hdf5Writer::createEvents( H5::H5File file, const SignalSet& data ) {
  H5::Group events = file.createGroup( "Events" );
  H5::Group grp = events.createGroup( "Times" );
  H5::Group wavetimes = grp.createGroup( "Waveforms" );
  H5::Group vittimes = grp.createGroup( "Vitals" );

  std::map<long, time_t> segmentsizes = data.offsets( );
  if ( !segmentsizes.empty( ) ) {
    hsize_t dims[] = { segmentsizes.size( ), 2 };
    H5::DataSpace space( 2, dims );

    H5::DataSet ds = events.createDataSet( "Segment Offsets",
        H5::PredType::STD_I64LE, space );
    long indexes[segmentsizes.size( ) * 2] = { 0 };
    int row = 0;
    for ( const auto& e : segmentsizes ) {
      indexes[ 2 * row ] = e.second;
      indexes[2 * row + 1] = e.first;
      row++;
    }
    ds.write( indexes, H5::PredType::STD_I64LE );
    writeAttribute( ds, "Columns", "time, segment offset" );
    writeAttribute( ds, "Timezone", "UTC" );
  }

  for ( const std::unique_ptr<SignalData>& m : data.allsignals( ) ) {
    // output() << "writing times for " << m->name( ) << std::endl;
    std::vector<time_t> times( m->times( ).rbegin( ), m->times( ).rend( ) );

    H5::Group * mygrp = ( m->wave( ) ? &wavetimes : &vittimes );

    if ( m->wave( ) ) {
      std::vector<time_t> alltimes;
      alltimes.reserve( times.size( ) );

      for ( auto& t : times ) {
        alltimes.push_back( t );
      }
      times = alltimes;
    }

    hsize_t dims[] = { times.size( ), 1 };
    H5::DataSpace space( 2, dims );

    H5::DataSet ds = mygrp->createDataSet( m->name( ).c_str( ),
        H5::PredType::STD_I64LE, space );
    ds.write( &times[0], H5::PredType::STD_I64LE );
    writeAttribute( ds, "Time Source", "raw" );
    writeAttribute( ds, SignalData::VALS_PER_DR, m->valuesPerDataRow( ) );
  }
}

int Hdf5Writer::initDataSet( const std::string& directory, const std::string& namestart,
    int compression ) {
  tempfileloc = directory + namestart;
  this->compression = compression;
  return 0;
}

int Hdf5Writer::drain( SignalSet& info ) {
  dataptr = &info;
  return 0;
}

std::vector<std::string> Hdf5Writer::closeDataSet( ) {
  SignalSet& data = *dataptr;

  time_t firstTime = data.earliest( );
  time_t lastTime = data.latest( );

  //  std::vector<time_t> alltimes = SignalUtils::alltimes( data );
  //  for ( const auto& m : data.waves( ) ) {
  //    std::cout << m.first << std::endl;
  //    const auto& s = m.second;
  //    std::vector<size_t> indi = SignalUtils::index( alltimes, *s );
  //    for ( size_t i : indi ) {
  //      std::cout << "  " << i << std::endl;
  //    }
  //  }

  std::vector<std::string> ret;
  std::string outy = tempfileloc + getDateSuffix( firstTime ) + ".hdf5";

  if ( data.vitals( ).empty( ) && data.waves( ).empty( ) ) {
    std::cerr << "Nothing to write to " << outy << std::endl;
    return ret;
  }

  output( ) << "Writing to " << outy << std::endl;

  H5::H5File file( outy, H5F_ACC_TRUNC );
  writeFileAttributes( file, data.metadata( ), firstTime, lastTime );

  createEvents( file, data );

  H5::Group grp = file.createGroup( "Vital Signs" );

  output( ) << "Writing " << data.vitals( ).size( ) << " Vitals" << std::endl;
  for ( auto& vits : data.vitals( ) ) {
    output( ) << "Writing Vital: " << vits.first << std::endl;
    std::vector<std::string> extras = vits.second->extras( );
    hsize_t sz = vits.second->size( );
    hsize_t dims[] = { sz, 1 + extras.size( ) };
    H5::DataSpace space( 2, dims );
    H5::DSetCreatPropList props;
    if ( compression > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      autochunk( dims, 2, chunkdims );
      props.setChunk( 2, chunkdims );
      props.setDeflate( compression );
    }
    H5::DataSet ds = grp.createDataSet( vits.first, H5::PredType::STD_I16LE, space, props );
    writeAttributes( ds, *( vits.second ) );

    std::string cols = "scaled value";
    for ( auto& x : extras ) {
      cols.append( ", " ).append( x );
    }

    writeAttribute( ds, "Columns", cols );
    writeAttribute( ds, "Data Label", vits.first );
    writeVital( ds, space, *( vits.second ) );
  }

  grp = file.createGroup( "Waveforms" );
  output( ) << "Writing " << data.waves( ).size( ) << " Waveforms" << std::endl;
  for ( auto& wavs : data.waves( ) ) {
    output( ) << "Writing Wave: " << wavs.first;
    int hz = wavs.second->metad( ).at( SignalData::HERTZ );
    auto st = std::chrono::high_resolution_clock::now( );

    hsize_t sz = wavs.second->size( );
    hsize_t dims[] = { sz * hz * 2, 1 }; // each datarow is 2s long, at hz values/s
    H5::DataSpace space( 2, dims );
    H5::DSetCreatPropList props;
    if ( compression > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      autochunk( dims, 2, chunkdims );
      props.setChunk( 2, chunkdims );
      props.setDeflate( compression );
    }
    H5::DataSet ds = grp.createDataSet( wavs.first, H5::PredType::STD_I16LE, space, props );
    writeAttributes( ds, *( wavs.second ) );
    writeAttribute( ds, "Columns", "value" );
    writeAttribute( ds, "Data Label", wavs.first );

    writeWave( ds, space, *( wavs.second ) );

    auto en = std::chrono::high_resolution_clock::now( );
    std::chrono::duration<float> dur = en - st;
    output( ) << " (complete in " << dur.count( ) << "ms)" << std::endl;
  }

  ret.push_back( outy );
}
