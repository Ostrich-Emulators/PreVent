/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WfdbWriter.h"
#include <cstdio>
#include <iostream>
#include <set>
#include <unistd.h>
#include <ctime>
#include <sstream>

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
  int x = chdir( directory.c_str( ) );

  files.clear( );

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
  int x = chdir( currdir.c_str( ) );
  wfdbquit( );
  return files;
}

int WfdbWriter::drain( SignalSet& info ) {
  std::map<double, std::vector<std::unique_ptr < SignalData>>> freqgroups;
  for ( auto& ds : info.vitals( ) ) {
    double freq = ds.second->hz( );
    freqgroups[freq].push_back( std::move( ds.second ) );
  }

  for ( auto& ds : info.waves( ) ) {
    double freq = ds.second->hz( );
    freqgroups[freq].push_back( std::move( ds.second ) );
  }

  for ( auto& ds : freqgroups ) {
    write( ds.first, ds.second );
  }

  return 0;
}

int WfdbWriter::write( double freq, std::vector<std::unique_ptr<SignalData>>&data ) {
  setsampfreq( freq );

  sigmap.clear( );
  for ( auto& signal : data ) {
    const std::string& name = signal->name( );
    sigmap[name].units = (char *) signal->uom( ).c_str( );
    sigmap[name].group = 0;
    sigmap[name].desc = (char *) name.c_str( );
    sigmap[name].fmt = 16;
  }

  time_t firstTime = SignalUtils::firstlast( data );

  auto synco = sync( data );

  std::string suffix = ( freq < 1 ? "vitals" : std::to_string( (int)freq ) + "hz" );
  std::string datedfile = fileloc + getDateSuffix( firstTime, "_" );
  std::string datafile = datedfile + "_" + suffix + ".dat";
  std::string headerfile = datedfile + "_" + suffix + ".hea";
  files.push_back( datafile );
  files.push_back( headerfile );

  WFDB_Siginfo sigs[sigmap.size( )];
  int i = 0;
  for ( const auto& signal : data ) {
    sigs[i] = sigmap[signal->name( )];
    sigs[i].fname = (char *) datafile.c_str( );
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

  newheader( (char *) headerfile.c_str( ) ); // we know this is a valid record name
  return 0;
}

std::vector<std::vector<WFDB_Sample>> WfdbWriter::sync(
    std::vector<std::unique_ptr<SignalData>>&olddata ) {

  int idx = 0;
  std::map<int, bool> wavemap;
  for ( auto& m : olddata ) {
    wavemap[idx++] = m->wave( );
  }

  std::vector<std::vector < std::string>> data = SignalUtils::syncDatas( olddata );

  std::vector<std::vector < WFDB_Sample>> ret;
  idx = 0;
  for ( std::vector<std::string> row : data ) {
    std::vector<WFDB_Sample> vec;

    for ( std::string rowcols : row ) {
      if ( wavemap[idx ] ) {
        std::stringstream stream( rowcols );
        for ( std::string each; std::getline( stream, each, ',' ); ) {
          vec.push_back( std::stoi( each ) );
        }
      }
      else {
        vec.push_back( std::stoi( rowcols ) );
      }
    }
    ret.push_back( vec );
    idx++;
  }

  return ret;
}
