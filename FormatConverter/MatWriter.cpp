/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "MatWriter.h"
#include <limits>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <bitset>

#include "config.h"
#include "SignalSet.h"
#include "SignalData.h"
#include "SignalUtils.h"

MatWriter::MatWriter( ) {
}

MatWriter::MatWriter( const MatWriter& ) {
}

MatWriter::~MatWriter( ) {
}

int MatWriter::initDataSet( const std::string& directory, const std::string& namestart, int comp ) {
  firsttime = std::numeric_limits<time_t>::max( );
  fileloc = directory + namestart;
  compression = ( 0 == comp ? MAT_COMPRESSION_NONE : MAT_COMPRESSION_ZLIB );

  char fulldatetime[sizeof "Thu Nov 31 10:10:27 1997"];
  time_t now;
  time( &now );
  std::strftime( fulldatetime, sizeof fulldatetime, "%c", gmtime( &now ) );

  std::stringstream header;
  header << "MATLAB 5.0 MAT-file, Platform: " << osname
      << ", Created by: fmtcnv (rpb6eg@virginia.edu) on: "
      << fulldatetime;

  matfile = Mat_CreateVer( fileloc.c_str( ), header.str( ).c_str( ), MAT_FT_MAT5 );

  return !( matfile );
}

std::string MatWriter::closeDataSet( ) {
  writeVitals( dataptr->vitals( ) );
  writeWaves( dataptr->waves( ) );

  Mat_Close( matfile );

  std::ifstream src( fileloc, std::ios::binary );
  std::string matfile = fileloc + getDateSuffix( firsttime ) + ".mat";
  std::ofstream dst( matfile, std::ios::binary | std::ios::trunc );

  dst << src.rdbuf( );
  remove( fileloc.c_str( ) );

  src.close( );
  dst.close( );

  return matfile;
}

int MatWriter::drain( SignalSet& info ) {
  dataptr = &info;
  for ( auto& m : info.vitals( ) ) {
    time_t t = m.second->startTime( );
    if ( t < firsttime ) {
      firsttime = t;
    }
  }
  for ( auto& m : info.waves( ) ) {
    time_t t = m.second->startTime( );
    if ( t < firsttime ) {
      firsttime = t;
    }
  }

  return 0;
}

int MatWriter::writeVitals( std::map<std::string, std::unique_ptr<SignalData>>&map ) {
  time_t earliest;
  time_t latest;
  SignalUtils::firstlast( map, &earliest, &latest );
  std::vector<std::string> labels;
  int maxlabelsize = 0;

  float freq = map.begin( )->second->metad( ).at( SignalData::HERTZ );
  const int timestep = ( freq < 1 ? 1 / freq : 1 );

  std::map<std::string, int> scales;
  for ( auto& m : map ) {
    labels.push_back( m.first );
    scales[m.first] = m.second->scale( );
    if ( m.first.size( ) > maxlabelsize ) {
      maxlabelsize = m.first.size( );
    }
  }

  std::vector<std::vector < std::string>> syncd = SignalUtils::syncDatas( map );
  size_t dims[2] = { syncd.size( ), map.size( ) };

  const int rows = syncd.size( );
  const int cols = map.size( );
  int timestamps[rows] = { 0 };
  short vitals[rows * cols];
  int row = 0;
  timestamps[0] = earliest;
  for ( std::vector<std::string> rowcols : syncd ) {
    int col = 0;
    for ( std::string valstr : rowcols ) {
      short val = ( scales[labels[col]] > 1
          ? short( std::stof( valstr ) * scales[labels[col]] )
          : short( std::stoi( valstr ) ) );

      // WARNING: we're transposing these values
      // (because that's how matio wants them)!
      vitals[col * rows + row] = val;
      col++;
    }
    row++;
    timestamps[row] = timestamps[row - 1] + timestep;
  }

  matvar_t * var = Mat_VarCreate( "vitals", MAT_C_INT16, MAT_T_INT16, 2, dims,
      vitals, 0 );

  Mat_VarWrite( matfile, var, compression );
  Mat_VarFree( var );

  // timestamps
  dims[0] = rows;
  dims[1] = 1;

  var = Mat_VarCreate( "vt", MAT_C_INT32, MAT_T_INT32, 2, dims, timestamps, 0 );
  Mat_VarWrite( matfile, var, compression );
  Mat_VarFree( var );

  // scales
  dims[0] = 1;
  dims[1] = cols;
  short scalesarr[cols];
  for ( int i = 0; i < cols; i++ ) {
    scalesarr[i] = scales[labels[i]];
  }
  var = Mat_VarCreate( "vscales", MAT_C_INT16, MAT_T_INT16, 2, dims, scalesarr, 0 );
  Mat_VarWrite( matfile, var, compression );
  Mat_VarFree( var );

  // FIXME: labels (and metadata?)
  //  dims[0] = 2;
  //  dims[1] = 2;
  //  char * vlabels[2] = { "x1", "y2" };
  //  //  for ( int i = 0; i < labels.size( ); i++ ) {
  //  //    vlabels[i] = labels[i].c_str( );
  //  //  }
  //  var = Mat_VarCreate( "vlabels", MAT_C_CHAR, MAT_T_UTF8, 2, dims, vlabels, 0 );
  //  int x = Mat_VarWrite( matfile, var, compression );
  //
  //  Mat_VarFree( var );

  return 0;
}

int MatWriter::writeWaves( std::map<std::string, std::unique_ptr<SignalData>>&data ) {
  return 0;
}
