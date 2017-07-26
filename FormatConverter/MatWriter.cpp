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
#include "ReadInfo.h"
#include "SignalData.h"

MatWriter::MatWriter( ) {
}

MatWriter::MatWriter( const MatWriter& ) {
}

MatWriter::~MatWriter( ) {
}

int MatWriter::initDataSet( const std::string& directory, const std::string& namestart, int ) {
  firsttime = std::numeric_limits<time_t>::max( );
  fileloc = directory + namestart;
  out.open( fileloc, std::ios::out | std::ios::trunc | std::ios::binary );

  writeheader( );

  return 0;
}

void MatWriter::writeheader( ) {
  char fulldatetime[sizeof "Thu Nov 31 10:10:27 1997"];
  time_t now;
  time( &now );
  std::strftime( fulldatetime, sizeof fulldatetime, "%c", gmtime( &now ) );

  std::stringstream ss;
  ss << "MATLAB 5.0 MAT-file, Platform: " << osname
      << ", Created by: fmtcnv (rpb6eg@virginia.edu) on: "
      << fulldatetime;
  std::string str( ss.str( ) );

  char header[124];
  memset( header, ' ', 124 );
  int sz = str.copy( header, 116 );
  if ( sz > 115 ) {
    sz = 115;
  }
  header[sz] = '\0';

  out << header;
  for ( int i = sz + 1; i < 125; i++ ) {
    out << ' ';
  }

  out.put( 0x01 );
  out.put( 0x00 );
  out << 'M' << 'I';
}

std::string MatWriter::closeDataSet( ) {
  writeVitals( dataptr->vitals( ) );
  writeWaves( dataptr->waves( ) );

  out.close( );

  std::ifstream src( fileloc, std::ios::binary );
  std::string matfile = fileloc + getDateSuffix( firsttime ) + ".mat";
  std::ofstream dst( matfile, std::ios::binary | std::ios::trunc );

  dst << src.rdbuf( );
  remove( fileloc.c_str( ) );

  src.close( );
  dst.close( );

  return matfile;
}

int MatWriter::drain( ReadInfo& info ) {
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

int MatWriter::writeVitals( std::map<std::string, std::unique_ptr<SignalData>>&data ) {
  //std::bitset header(32);
  std::stringstream dt;
  dt << 3;


  return 0;
}

int MatWriter::writeWaves( std::map<std::string, std::unique_ptr<SignalData>>&data ) {
  return 0;
}
