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

#include <hdf5.h>
#include <iostream>
#include <utility>
#include <chrono>
#include <sstream>

#include "config.h"
#include "Hdf5Writer.h"
#include "DataSetDataCache.h"
#include "DataRow.h"

const int Hdf5Writer::MISSING_VALUE = -32768;
const std::set<std::string> Hdf5Writer::Hz60({ "RR", "VNT_PRES", "VNT_FLOW" } );
const std::set<std::string> Hdf5Writer::Hz120({ "ICP1", "ICP2", "ICP4", "LA4" } );

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

void Hdf5Writer::writeAttributes( H5::H5Location& loc,
        const time_t& start, const time_t& end ) {
  writeAttribute( loc, "Timezone", "UTC" );
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
    duration += std::to_string( t->tm_yday ) + " days,";
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

void Hdf5Writer::writeAttributes( H5::DataSet& ds, const DataSetDataCache& data,
        double period, double freq ) {
  writeAttributes( ds, data.startTime( ), data.endTime( ) );
  writeAttribute( ds, "Scale", double( 1.0 / data.scale( ) ) );
  writeAttribute( ds, "Sample Period", period );
  writeAttribute( ds, "Sample Frequency", freq );
  writeAttribute( ds, "Sample Period Unit", "second" );
  writeAttribute( ds, "Sample Frequency Unit", "Hz" );
  writeAttribute( ds, "Unit of Measure", data.uom( ) );
  writeAttribute( ds, "Missing Value Marker", MISSING_VALUE );
}

void Hdf5Writer::writeAttributes( H5::H5File file,
        std::map<std::string, std::string> datasetattrs,
        const time_t& firstTime, const time_t& lastTime ) {

  writeAttributes( file, firstTime, lastTime );

  for ( std::map<std::string, std::string>::const_iterator it = datasetattrs.begin( );
          it != datasetattrs.end( ); ++it ) {
    writeAttribute( file, it->first, it->second );
  }
}

void Hdf5Writer::writeVital( H5::DataSet& ds, H5::DataSpace& space,
        DataSetDataCache& data ) {
  const int rows = data.size( );
  // MSVC doesn't allow 2D arrays with variable size indices (bombs during initialization)
  // So we'll just use a 1D array, and populate it like a 2D array
  const int buffsz = rows * 4;
  int * buffer = new int[buffsz];
  int scale = data.scale( );
  data.startPopping( );
  for ( int i = 0; i < rows; i++ ) {
    std::unique_ptr<DataRow> row = std::move( data.pop( ) );
    int idx = i * 4;
    buffer[idx] = (int) ( row->time );
    buffer[idx + 1] = (int) ( std::stof( row->data ) * scale );
    buffer[idx + 2] = (int) ( row->high.empty( ) ? MISSING_VALUE : scale * std::stof( row->high ) );
    buffer[idx + 3] = (int) ( row->low.empty( ) ? MISSING_VALUE : scale * std::stof( row->low ) );
  }

  ds.write( buffer, H5::PredType::STD_I32LE );
  delete[] buffer;
}

void Hdf5Writer::writeWave( H5::DataSet& ds, H5::DataSpace& space,
        DataSetDataCache& data, int hz ) {

  // WARNING: --------------------------------------------------------- //
  // WARNING: the data in the DataSetCache is always sampled at 240Hz - //
  // WARNING: but some waves are up-sampled, so we must fix the       - //
  // WARNING: sampling rate here before writing the data to the file  - //
  // WARNING: --------------------------------------------------------- //

  int rows = data.size( );
  const hsize_t maxslabcnt = 40960;
  hsize_t offset[] = { 0, 0 };
  hsize_t count[] = { 0, 3 };

  // see the comment in writeVital
  int * buffer = new int[maxslabcnt * 3];
  int idx = 0;

  // We're keeping a buffer to eventually write to the file. However, this 
  // buffer is based on a number of rows, *not* some multiple of the data size.
  // This means we are really keeping two counters going at all times, and 
  // once we're out of all the loops, we need to write any residual data.

  data.startPopping( );
  for ( int i = 0; i < rows; i++ ) {
    std::unique_ptr<DataRow> row = data.pop( );
    std::unique_ptr<std::vector<int>> lst = resample( row->data, hz );

    int hzcount = 0;
    for ( int val : *lst ) {
      long pos = idx * 3;
      // after hz values, increment the time by one second
      buffer[pos] = (long) ( hzcount++ >= hz ? row->time + 1 : row->time );
      buffer[pos + 1] = val;
      buffer[pos + 2] = ( offset[0] + idx ) % hz;
      idx++;
      // std::cout << "idx is " << idx << std::endl;

      if ( idx == maxslabcnt ) {
        count[0] = idx;
        space.selectHyperslab( H5S_SELECT_SET, count, offset );
        offset[0] += count[0];
        idx = 0;

        H5::DataSpace memspace( 2, count );
        ds.write( buffer, H5::PredType::STD_I32LE, memspace, space );
      }
    }
  }

  // finally, write whatever's left in the buffer
  if ( 0 != idx ) {
    count[0] = idx;
    space.selectHyperslab( H5S_SELECT_SET, count, offset );
    offset[0] += count[0];
    idx = 0;

    H5::DataSpace memspace( 2, count );
    ds.write( buffer, H5::PredType::STD_I32LE, memspace, space );
  }
  delete[] buffer;
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

int Hdf5Writer::flush( const std::string& outputdir, const std::string& prefix, int compression,
        const time_t& firstTime, const time_t& lastTime, int ordinal,
        const std::map<std::string, std::string>& datasetattrs,
        std::map<std::string, std::unique_ptr<DataSetDataCache>>&vitals,
        std::map<std::string, std::unique_ptr<DataSetDataCache>>&waves ) {
  std::string output( outputdir );

  size_t extpos = outputdir.find_last_of( dirsep, outputdir.length( ) );
  if ( outputdir.length( ) - 1 != extpos || extpos < 0 ) {
    // doesn't end with a dirsep, so add it
    output += dirsep;
  }

  tm * time = gmtime( &firstTime );
  char buf[sizeof "-YYYYMMDD.hdf5"];
  strftime( buf, sizeof buf, "-%Y%m%d.hdf5", time );
  output += prefix + "p" + std::to_string( ordinal ) + buf;

  if ( vitals.empty( ) && waves.empty( ) ) {
    std::cout << "Nothing to write to " << output << std::endl;
    return 1;
  }

  std::cout << "Writing to " << output << std::endl;

  H5::H5File file( output, H5F_ACC_TRUNC );
  writeAttributes( file, datasetattrs, firstTime, lastTime );

  H5::Group grp = file.createGroup( "Vital Signs" );

  std::cout << "Writing " << vitals.size( ) << " Vitals" << std::endl;
  for ( std::map<std::string, std::unique_ptr < DataSetDataCache>>::iterator it
          = vitals.begin( ); it != vitals.end( ); ++it ) {
    std::cout << "Writing Vital: " << it->first << std::endl;
    hsize_t sz = it->second->size( );
    hsize_t dims[] = { sz, 4 };
    H5::DataSpace space( 2, dims );
    H5::DSetCreatPropList props;
    if ( compression > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      autochunk( dims, 2, chunkdims );
      props.setChunk( 2, chunkdims );
      props.setDeflate( compression );
    }
    H5::DataSet ds = grp.createDataSet( it->first, H5::PredType::STD_I32LE, space, props );
    writeAttributes( ds, *( it->second ), 2.0, 1.0 / 2.0 );
    writeAttribute( ds, "Columns", "timestamp, scaled value, scaled high alarm, scaled low alarm" );
    writeAttribute( ds, "Vital Label", it->first );
    writeVital( ds, space, *it->second );
  }

  grp = file.createGroup( "Waveforms" );
  std::cout << "Writing " << waves.size( ) << " Waveforms" << std::endl;
  for ( std::map<std::string, std::unique_ptr < DataSetDataCache>>::iterator it
          = waves.begin( ); it != waves.end( ); ++it ) {
    std::cout << "Writing Waveform: " << it->first;
    int hz = getHertz( it->first );
    auto st = std::chrono::high_resolution_clock::now( );

    hsize_t sz = it->second->size( );
    hsize_t dims[] = { sz * hz * 2, 3 }; // 2 second blocks, so we have double the hz values
    H5::DataSpace space( 2, dims );
    H5::DSetCreatPropList props;
    if ( compression > 0 ) {
      hsize_t chunkdims[] = { 0, 0 };
      autochunk( dims, 2, chunkdims );
      props.setChunk( 2, chunkdims );
      props.setDeflate( compression );
    }
    H5::DataSet ds = grp.createDataSet( it->first, H5::PredType::STD_I32LE, space, props );
    writeAttributes( ds, *( it->second ), 1.0 / hz, hz );
    writeAttribute( ds, "Columns", "timestamp, value, sample number" );
    writeAttribute( ds, "Waveform Label", it->first );

    writeWave( ds, space, *it->second, hz );

    auto en = std::chrono::high_resolution_clock::now( );
    std::chrono::duration<float> dur = en - st;
    std::cout << " (complete in " << dur.count( ) << "ms)" << std::endl;
  }

  return 0;
}

int Hdf5Writer::getHertz( const std::string& wavename ) {
  if ( 0 != Hz60.count( wavename ) ) {
    return 60;
  }

  if ( 0 != Hz120.count( wavename ) ) {
    return 120;
  }

  return 240;
}

std::unique_ptr<std::vector<int>> Hdf5Writer::resample( const std::string& data, int hz ) {
  /**
  The data arg is always sampled at 240 Hz and is in a 2-second block (480 vals)
  However, the actual wave sampling rate may be less (will always be a multiple of 60Hz)
  so we need to remove the extra values if the values are up-sampled
   **/

  std::unique_ptr<std::vector<int>> samples( new std::vector<int> );
  samples->reserve( 2 * hz );
  std::stringstream stream( data );
  if ( 240 == hz ) {
    // we need every value
    //std::cout<<"480 vals extracted (240hz)"<<std::endl;
    for ( std::string each; std::getline( stream, each, ',' ); samples->push_back( std::stoi( each ) ) );
  }
  else if ( 120 == hz ) {
    // remove every other value
    for ( std::string each; std::getline( stream, each, ',' ); samples->push_back( std::stoi( each ) ) ) {
      std::getline( stream, each, ',' );
    }
  }
  else if ( 60 == hz ) {
    // keep one val, skip the next 3
    for ( std::string each; std::getline( stream, each, ',' ); samples->push_back( std::stoi( each ) ) ) {
      int offset = each.length( )*3 + 3; // +3 for the commas
      stream.seekg( offset, std::ios_base::cur );
    }
  }

  return samples;
}
