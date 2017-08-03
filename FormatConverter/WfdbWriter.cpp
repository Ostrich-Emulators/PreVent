/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WfdbWriter.h"
#include <limits>
#include <cstdio>
#include <iostream>
#include <set>
#include <unistd.h>
#include <ctime>

#include "SignalSet.h"
#include "SignalData.h"
#include "SignalUtils.h"

WfdbWriter::WfdbWriter( ) {
}

WfdbWriter::WfdbWriter( const WfdbWriter& ) {
}

WfdbWriter::~WfdbWriter( ) {
}

int WfdbWriter::initDataSet( const std::string& directory, const std::string& namestart, int ) {
  currdir = getcwd( NULL, 0 );
  chdir( directory.c_str( ) );

  sigmap.clear( );

  fileloc = namestart;
  std::string rectest = namestart + "-20170101"; // we're going to replace the date
  int headerok = newheader( (char *) rectest.c_str( ) ); // check that our header file is valid
  if ( 0 == headerok ) {
    // remove the file
    std::string header = directory + rectest + ".hea";
    remove( header.c_str( ) );
  }
  return headerok;
}

std::vector<std::string> WfdbWriter::closeDataSet( ) {
  chdir( currdir.c_str( ) );
  wfdbquit( );

  std::vector<std::string> ret;
  ret.push_back( fileloc + ".hea" );
  return ret;
}

int WfdbWriter::drain( SignalSet& info ) {
  auto& vitals = info.vitals( );
  auto& waves = info.waves( );

  return ( vitals.empty( )
      ? write( waves )
      : write( vitals ) );
}

int WfdbWriter::write( std::map<std::string, std::unique_ptr<SignalData>>&data ) {
  for ( auto& vit : data ) {
    if ( 0 == sigmap.count( vit.first ) ) {
      sigmap[vit.first].units = (char *) vit.second->uom( ).c_str( );
      sigmap[vit.first].group = 0;
      sigmap[vit.first].desc = (char *) vit.first.c_str( );
      sigmap[vit.first].fmt = 16;

      if ( 0 != vit.second->metad( ).count( SignalData::HERTZ ) ) {
        setsampfreq( vit.second->metad( )[SignalData::HERTZ] );
      }
    }
  }

  time_t firstTime = SignalUtils::firstlast( data );

  std::vector<std::string> labels;
  auto synco = sync( data, labels );

  fileloc += getDateSuffix( firstTime );
  std::string output = fileloc + ".dat";

  WFDB_Siginfo sigs[sigmap.size( )];
  int i = 0;
  for ( const std::string& name : labels ) {
    sigs[i] = sigmap[name];
    sigs[i].fname = (char *) output.c_str( );
    i++;
  }

  if ( osigfopen( sigs, sigmap.size( ) ) < sigmap.size( ) ) {
    return -1;
  }

  tm * t = gmtime( &firstTime );
  if ( 0 != ( t->tm_hour + t->tm_min + t->tm_sec ) ) { // not 00:00:00 (midnight)?
    char timestr[sizeof "00:00:00"];
    std::strftime( timestr, sizeof timestr, "%T", t );
    setbasetime( timestr );
  }

  for ( const auto& vec : synco ) {
    putvec( (WFDB_Sample *) ( &vec[0] ) );
  }

  newheader( (char *) fileloc.c_str( ) ); // we know this is a valid record name
  return 0;
}

std::vector<std::vector<WFDB_Sample>> WfdbWriter::sync( std::map<std::string,
    std::unique_ptr<SignalData>>&olddata, std::vector<std::string>& labels ) {

  std::vector<std::vector < std::string>> data = SignalUtils::syncDatas( olddata );

  for ( const auto& map : olddata ) {
    labels.push_back( map.first );
  }

  std::vector<std::vector < WFDB_Sample>> ret;
  for ( std::vector<std::string> row : data ) {
    std::vector<WFDB_Sample> vec;

    for ( std::string rowcols : row ) {
      vec.push_back( std::stoi( rowcols ) );
    }
    ret.push_back( vec );
  }

  return ret;
}
