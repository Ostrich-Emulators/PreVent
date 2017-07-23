/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "ZlWriter.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <limits>
#include <fstream>

#include "ReadInfo.h"
#include "SignalData.h"

ZlWriter::ZlWriter( ) {
}

ZlWriter::ZlWriter( const ZlWriter& ) {
}

ZlWriter::~ZlWriter( ) {
}

int ZlWriter::initDataSet( const std::string& directory, const std::string& namestart,
    int compression ) {
  filestart = directory + namestart;
  return 0;
}

std::string ZlWriter::closeDataSet( ) {
  return filename;
}

int ZlWriter::drain( ReadInfo& info ) {
  time_t firsttime = std::numeric_limits<time_t>::max( );

  std::unique_ptr<DataRow> vits[info.vitals( ).size( )];
  std::unique_ptr<DataRow> wavs[info.waves( ).size( )];
  std::vector<std::string> vls;
  std::vector<std::string> ws;

  int vidx = 0;
  for ( auto& map : info.vitals( ) ) {
    vls.push_back( map.first );
    map.second->startPopping( );
    vits[vidx++] = std::move( map.second->pop( ) );

    if ( map.second->startTime( ) < firsttime ) {
      firsttime = map.second->startTime( );
    }
  }

  int widx = 0;
  for ( auto& map : info.waves( ) ) {
    vls.push_back( map.first );
    map.second->startPopping( );
    vits[widx++] = std::move( map.second->pop( ) );

    if ( map.second->startTime( ) < firsttime ) {
      firsttime = map.second->startTime( );
    }
  }

  char recsuffix[sizeof "-YYYYMMDD"];
  std::strftime( recsuffix, sizeof recsuffix, "-%Y%m%d", gmtime( &firsttime ) );
  filename = filestart + recsuffix;

  std::ofstream out( filename );
  out << "HEADER" << std::endl;
  for ( auto& m : info.metadata( ) ) {
    out << m.first << "=" << m.second << std::endl;
  }

  //for ( auto& m : info.vitals( ) ) {

  //}

  out.close( );


  return 0;
}
