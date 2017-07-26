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

std::string WfdbWriter::closeDataSet( ) {
  chdir( currdir.c_str( ) );
  wfdbquit( );
  return fileloc + ".hea";
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

  time_t firstTime = std::numeric_limits<time_t>::max( );

  std::vector<std::string> labels;
  auto synco = sync( data, labels, firstTime );

  char recsuffix[sizeof "-YYYYMMDD"];
  std::strftime( recsuffix, sizeof recsuffix, "-%Y%m%d", gmtime( &firstTime ) );
  fileloc += recsuffix;
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
    std::unique_ptr<SignalData>>&data, std::vector<std::string>& labels, time_t& earliest ) {
  earliest = std::numeric_limits<time_t>::max( );

  time_t latest = 0;
  for ( auto& map : data ) {
    labels.push_back( map.first );

    if ( map.second->startTime( ) < earliest ) {
      earliest = map.second->startTime( );
    }
    if ( map.second->endTime( ) > latest ) {
      latest = map.second->endTime( );
    }
  }

  const int cols = data.size( );
  int rows = rowForTime( earliest, latest, 2 ) + 1; // +1 for last time

  // WARNING: this logic assumes the largest signal data starts earliest
  // and ends latest. This isn't necessarily try, as one signal might
  // start very early, and a different one might end very late, resulting
  // in more indicies for empty data points

  // we'll make a 1D array, but treat it like a 2D array. 
  // We'll convert to the vectors at the end.
  const int buffsz = rows * cols;
  //  std::cout << "data buffer is " << rows << "*" << cols << "=" << buffsz << std::endl;
  int buffer[buffsz];

  for ( int i = 0; i < buffsz; i++ ) {
    buffer[i] = -32768;
  }

  // start a-poppin'!
  // our strategy is to pop from all signals at the same time, and figure out
  // which array index the time corresponds to.
  int emptycnt = 0;
  int empties[cols];
  for ( int col = 0; col < cols; col++ ) {
    empties[col] = 0;
  }
  // run through one time to figure out our earliest time
  int firstvals[cols];
  int firsttimes[cols];
  for ( int col = 0; col < cols; col++ ) {
    std::string name = labels[col];

    data[name]->startPopping( );
    if ( data[name]->size( ) > 0 ) {
      const auto& row = data[name]->pop( );
      firsttimes[col] = row->time;
      firstvals[col] = std::stoi( row->data );

      if ( firsttimes[col] < earliest ) {
        earliest = firsttimes[col];
      }
    }
    else {
      if ( 0 != empties[col] ) {
        empties[col]++;
        emptycnt++;
      }
      //      std::cout << std::endl;
    }
  }

  for ( int col = 0; col < cols; col++ ) {
    int row4time = rowForTime( earliest, firsttimes[col], 2 );
    int index = ( row4time * cols ) + col;
    //    std::cout << labels[col] << "[" << index << "] " << firsttimes[col] << ": " << firstvals[col] << std::endl;
    buffer[index] = firstvals[col];
  }

  while ( emptycnt < cols ) {
    for ( int col = 0; col < cols; col++ ) {
      std::string name = labels[col];
      //      std::cout << name << " " << data[name]->size( );
      if ( data[name]->size( ) > 0 ) {
        const auto& row = data[name]->pop( );
        time_t t = row->time;
        int v = std::stoi( row->data );

        int row4time = rowForTime( earliest, t, 2 );
        int index = ( row4time * cols ) + col;

        //        std::cout << " is idx[" << index << "] " << t << ": " << v << std::endl;
        //        if ( index > buffsz ) {
        //          std::cerr << "here!" << std::endl;
        //        }
        buffer[index] = v;
      }
      else {
        if ( 0 == empties[col] ) {
          empties[col]++;
          emptycnt++;
        }
        //        std::cout << std::endl;
      }
    }
  }

  std::vector<std::vector < WFDB_Sample>> ret;
  for ( int row = 0; row < rows; row++ ) {
    std::vector<WFDB_Sample> vec;
    for ( int col = 0; col < cols; col++ ) {
      vec.push_back( buffer[row * cols + col] );
    }
    ret.push_back( vec );
  }

  return ret;
}

int WfdbWriter::rowForTime( time_t starttime, time_t mytime, float timestep ) const {
  int diff = mytime - starttime;
  return diff / timestep;
}