/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WfdbWriter.h"
#include <limits>
#include <cstdio>

#include "ReadInfo.h"
#include "SignalData.h"

WfdbWriter::WfdbWriter( ) {
}

WfdbWriter::WfdbWriter( const WfdbWriter& ) {
}

WfdbWriter::~WfdbWriter( ) {
}

int WfdbWriter::initDataSet( const std::string& newfile, int compression ) {
  fileloc = newfile;
  sigmap.clear( );
  firstTime = std::numeric_limits<time_t>::max( );

  std::string rectest = fileloc + "-20170101"; // we're going to replace the date
  int headerok = newheader( (char *) rectest.c_str( ) ); // check that our header file is valid
  if ( 0 == headerok ) {
    // remove the file
    std::string header = rectest + ".hea";
    remove( header.c_str( ) );
  }
  return headerok;
}

std::string WfdbWriter::closeDataSet( ) {
  wfdbquit( );
  return fileloc;
}

int WfdbWriter::drain( ReadInfo& info ) {
  auto& vitals = info.vitals( );
  auto& waves = info.waves( );
  auto& metas = info.metadata( );

  for ( auto& vit : vitals ) {
    if ( 0 == sigmap.count( vit.first ) ) {
      sigmap[vit.first].units = (char *) vit.second->uom( ).c_str( );
      sigmap[vit.first].group = 0;
      sigmap[vit.first].desc = (char *) vit.first.c_str( );
      sigmap[vit.first].fmt = 16;
    }
  }

  std::vector<std::string> labels;
  auto synco = sync( vitals, labels, firstTime );

  char recsuffix[sizeof "-YYYYMMDD"];
  strftime( recsuffix, sizeof recsuffix, "-%Y%m%d", gmtime( &firstTime ) );
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

  char timestr[sizeof "01:01:01"];
  strftime( timestr, sizeof timestr, "%T", gmtime( &firstTime ) );
  setbasetime( timestr );

  for ( const auto& vec : synco ) {
    putvec( (WFDB_Sample *) ( &vec[0] ) );
  }

  newheader( (char *) fileloc.c_str( ) ); // we know this is a valid record name
  return 0;
}

std::vector<std::vector<WFDB_Sample>> WfdbWriter::sync( std::map<std::string, std::unique_ptr<SignalData>>&data,
    std::vector<std::string>& labels, time_t& earliest ) {
  earliest = std::numeric_limits<time_t>::max( );

  int rows = 0;
  for ( auto& map : data ) {
    labels.push_back( map.first );

    int sz = map.second->size( );
    if ( sz > rows ) {
      rows = sz;
    }

    map.second->startPopping( );
  }

  // WARNING: this logic assumes the largest signal data starts earliest
  // and ends latest. This isn't necessarily try, as one signal might
  // start very early, and a different one might end very late, resulting
  // in more indicies for empty data points


  // we'll make a 1D array, but treat it like a 2D array. 
  // We'll convert to the vectors at the end.
  const int cols = data.size( );
  const int buffsz = rows * cols;
  int buffer[buffsz];

  for ( int i = 0; i < buffsz; i++ ) {
    buffer[i] = -32768;
  }

  // start a-poppin'!
  // our strategy is to pop from all signals at the same time, and figure out
  // which array index the time corresponds to.
  int emptycnt = 0;

  // run through one time to figure out our earliest time
  int firstvals[cols];
  int firsttimes[cols];
  for ( int col = 0; col < cols; col++ ) {
    std::string name = labels[col];
    if ( data[name]->size( ) > 0 ) {
      const auto& row = data[name]->pop( );
      firsttimes[col] = row->time;
      firstvals[col] = std::stoi( row->data );

      if ( firsttimes[col] < earliest ) {
        earliest = firsttimes[col];
      }
    }
    else {
      emptycnt++;
    }
  }

  for ( int col = 0; col < cols; col++ ) {
    int row4time = rowForTime( earliest, firsttimes[col], 2 );
    buffer[row4time + col] = firstvals[col];
  }

  while ( emptycnt < cols ) {
    for ( int col = 0; col < cols; col++ ) {
      std::string name = labels[col];
      if ( data[name]->size( ) > 0 ) {
        const auto& row = data[name]->pop( );
        time_t t = row->time;
        int v = std::stoi( row->data );

        int row4time = rowForTime( earliest, t, 2 );
        buffer[row4time + col] = v;
      }
      else {
        emptycnt++;
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