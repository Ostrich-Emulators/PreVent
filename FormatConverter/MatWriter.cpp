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
#include <iomanip>
#include <vector>

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
  //compression = ( 0 == comp ? MAT_COMPRESSION_NONE : MAT_COMPRESSION_ZLIB );
  compression = MAT_COMPRESSION_NONE;

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

std::vector<std::string> MatWriter::closeDataSet( ) {
  Mat_Close( matfile );

  std::ifstream src( fileloc, std::ios::binary );
  std::string matfile = fileloc + getDateSuffix( firsttime ) + ".mat";
  std::ofstream dst( matfile, std::ios::binary | std::ios::trunc );

  dst << src.rdbuf( );
  remove( fileloc.c_str( ) );

  src.close( );
  dst.close( );
  std::vector<std::string> ret;
  ret.push_back( matfile );
  return ret;
}

int MatWriter::drain( SignalSet& info ) {
  dataptr = &info;
  firsttime = info.earliest( );

  writeVitals( info.vitals( ) );

  std::map<int, std::vector<std::unique_ptr < SignalData>>> freqgroups;
  for ( auto& ds : info.waves( ) ) {
    int freq = (int) ds.second->hz( );
    freqgroups[freq].push_back( std::move( ds.second ) );
  }

  for ( auto& ds : freqgroups ) {
    writeWaves( ds.first, ds.second );
  }

  return 0;
}

int MatWriter::writeStrings( const std::string& label, std::vector<std::string>& strings ) {
  const size_t rows = strings.size( );
  size_t cols = 0;
  for ( auto& s : strings ) {
    if ( s.size( ) > cols ) {
      cols = s.size( );
    }
  }

  // WARNING: matlab/matio needs column-major ordering
  size_t dims[] = { rows, cols };
  char strdata[cols][rows] = { };
  for ( int c = 0; c < cols; c++ ) {
    for ( int r = 0; r < rows; r++ ) {
      strdata[c][r] = ( c > strings[r].size( ) ? ' ' : strings[r][c] );
    }
  }

  matvar_t * var = Mat_VarCreate( label.c_str( ), MAT_C_CHAR, MAT_T_UTF8, 2,
      dims, strdata, 0 );

  // this is the code for writing cells, which seems more appropriate to me
  // 
  //  matvar_t * var = Mat_VarCreate( label.c_str( ), MAT_C_CELL, MAT_T_CELL, 2,
  //      dims, NULL, 0 );
  //
  //  for ( int i = 0; i < rows; i++ ) {
  //    size_t strdims[2] = { 1, strings[i].size( ) };
  //    char * text = (char *) strings[i].c_str( );
  //    matvar_t * vart = Mat_VarCreate( NULL, MAT_C_CHAR, MAT_T_UTF8, 2, strdims,
  //        text, 0 );
  //    Mat_VarSetCell( var, i, vart );
  //    // does vart get free'd when var does?
  //  }

  int ok = Mat_VarWrite( matfile, var, compression );
  Mat_VarFree( var );
  return ok;
}

int MatWriter::writeVitals( std::map<std::string, std::unique_ptr<SignalData>>&oldmap ) {
  time_t earliest;
  time_t latest;

  std::vector<std::unique_ptr < SignalData>> signals = SignalUtils::vectorize( oldmap );

  SignalUtils::firstlast( signals, &earliest, &latest );

  float freq = ( *signals.begin( ) )->hz( );
  const int timestep = ( freq < 1 ? 1 / freq : 1 );

  std::vector<std::string> labels;
  std::vector<std::string> uoms;
  std::map<std::string, int> scales;
  for ( auto& m : signals ) {
    labels.push_back( m->name( ) );
    scales[m->name( )] = m->scale( );
    uoms.push_back( m->uom( ) );
  }

  std::vector<std::vector < std::string>> syncd = SignalUtils::syncDatas( signals );
  const int rows = syncd.size( );
  const int cols = signals.size( );

  size_t dims[2] = { (size_t) rows, (size_t) cols };

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

  // units of measure
  writeStrings( "vuom", uoms );
  writeStrings( "vlabels", labels );

  // FIXME: (metadata?)

  return 0;
}

int MatWriter::writeWaves( const int& freq, std::vector<std::unique_ptr<SignalData>>&signals ) {
  time_t earliest;
  time_t latest;

  const std::string sfx = std::to_string( freq ) + "hz";

  SignalUtils::firstlast( signals, &earliest, &latest );

  const int timestep = 1;

  std::vector<std::string> labels;
  std::vector<std::string> uoms;
  for ( auto& m : signals ) {
    labels.push_back( m->name( ) );
    uoms.push_back( m->uom( ) );
  }

  std::vector<std::vector < std::string>> syncd = SignalUtils::syncDatas( signals );
  const size_t rows = syncd.size( ) * freq;
  const int cols = signals.size( );

  size_t dims[2] = { rows, (size_t) cols };
  matvar_t * var = Mat_VarCreate( std::string( "waves" + sfx ).c_str( ),
      MAT_C_INT16, MAT_T_INT16, 2, dims, NULL, 0 );
  Mat_VarWriteInfo( matfile, var );

  int start[2] = { 0, 0 };
  int stride[2] = { 1, 1 };
  int edge[2] = { freq, 1 };

  for ( std::vector<std::string> rowcols : syncd ) {
    for ( int col = 0; col < rowcols.size( ); col++ ) {
      start[1] = col;
      std::vector<short> slices = DataRow::shorts( rowcols[col] );
      Mat_VarWriteData( matfile, var, &slices[0], start, stride, edge );
    }

    start[0] += freq;
  }
  Mat_VarFree( var );

  // timestamps
  // use incremental writing here too, because if we have 24 hours of >240hz
  // samples, it's too much data to fit in memory
  dims[0] = rows;
  dims[1] = 1;
  start[0] = 0;
  start[1] = 0;
  int tschunk = freq;
  var = Mat_VarCreate( std::string( "wt" + sfx ).c_str( ),
      MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, NULL, 0 );
  Mat_VarWriteInfo( matfile, var );

  double timestamps[tschunk] = { 0 };
  double currenttime = earliest;
  double slicetime = 1.0 / (double) freq;

  for ( size_t row = 0; row < syncd.size( ); row++ ) {
    for ( int slice = 0; slice < freq; slice++ ) {
      timestamps[slice] = currenttime + slicetime*slice;
    }

    Mat_VarWriteData( matfile, var, timestamps, start, stride, edge );
    start[0] += freq;

    currenttime += timestep;
  }

  Mat_VarWrite( matfile, var, compression );
  Mat_VarFree( var );

  // units of measure
  writeStrings( "wuom" + sfx, uoms );
  writeStrings( "wlabels" + sfx, labels );

  // FIXME: (metadata?)

  return 0;
}
