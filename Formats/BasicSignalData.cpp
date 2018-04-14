/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataSetDataCache.cpp
 * Author: ryan
 * 
 * Created on August 3, 2016, 7:47 AM
 */

#include "BasicSignalData.h"
#include "DataRow.h"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <limits>
#include <queue>

const int BasicSignalData::CACHE_LIMIT = 15000;

BasicSignalData::BasicSignalData( const std::string& name, bool largefile, bool wavedata )
: label( name ), firstdata( std::numeric_limits<dr_time>::max( ) ), lastdata( 0 ),
datacount( 0 ), popping( false ), iswave( wavedata ), highval( std::numeric_limits<int>::min( ) ),
lowval( std::numeric_limits<int>::max( ) ) {
  //file = ( largefile ? fopen( std::tmpnam( nullptr ), "w+" ) : NULL );
  if ( largefile ) {
    std::string fname = "/tmp/cache-" + name + ".prevent";
    file = fopen( fname.c_str( ), "wb+" );
  }
  else {
    file = NULL;
  }
}

BasicSignalData::BasicSignalData( const BasicSignalData& orig ) : SignalData( orig ),
label( orig.label ), firstdata( orig.firstdata ), lastdata( orig.lastdata ),
datacount( orig.datacount ), popping( orig.popping ), iswave( orig.iswave ),
highval( orig.highval ), lowval( orig.lowval ) {
  for ( auto const& i : orig.data ) {
    data.push_front( std::unique_ptr<DataRow>( new DataRow( *i ) ) );
  }
}

BasicSignalData::~BasicSignalData( ) {
  data.clear( );
  if ( NULL != file ) {
    std::fclose( file );
  }
}

std::unique_ptr<SignalData> BasicSignalData::shallowcopy( bool includedates ) {
  std::unique_ptr<BasicSignalData> copy( new BasicSignalData( label, ( NULL != file ) ) );

  if ( includedates ) {
    copy->firstdata = this->firstdata;
    copy->lastdata = this->lastdata;
  }

  copy->metad( ).insert( metad( ).begin( ), metad( ).end( ) );
  copy->metai( ).insert( metai( ).begin( ), metai( ).end( ) );
  copy->metas( ).insert( metas( ).begin( ), metas( ).end( ) );
  copy->popping = this->popping;
  for ( std::string& s : extras( ) ) {
    copy->extras( ).push_back( s );
  }
  return std::move( copy );
}

const dr_time& BasicSignalData::startTime( ) const {
  return firstdata;
}

const dr_time& BasicSignalData::endTime( ) const {
  return lastdata;
}

std::unique_ptr<DataRow> BasicSignalData::pop( ) {
  if ( !popping ) {
    startPopping( );
  }

  if ( NULL != file && data.empty( ) ) {
    int lines = uncache( );
    if ( 0 == lines ) {
      std::fclose( file );
      file = NULL;
    }
  }

  datacount--;
  std::unique_ptr<DataRow> row = std::move( data.front( ) );
  data.pop_front( );
  dates.pop_front( );
  return row;
}

size_t BasicSignalData::size( ) const {
  return datacount;
}

const std::string& BasicSignalData::name( ) const {
  return label;
}

void BasicSignalData::startPopping( ) {
  popping = true;
  if ( NULL != file ) {
    cache( ); // copy any extra rows to disk
    std::rewind( file );
  }
}

int BasicSignalData::highwater( ) const {
  return highval;
}

int BasicSignalData::lowwater( ) const {
  return lowval;
}

void BasicSignalData::cache( ) {
  while ( !data.empty( ) ) {
    std::unique_ptr<DataRow> a = std::move( data.front( ) );
    data.pop_front( );

    std::stringstream ss;
    ss << a->time << " " << a->data << " " << a->high << " " << a->low << " ";
    if ( !a->extras.empty( ) ) {
      for ( const auto& x : a->extras ) {
        ss << "|" << x.first << "=" << x.second;
      }
    }
    ss << "\n";
    std::fputs( ss.str( ).c_str( ), file );
  }
}

void BasicSignalData::add( const DataRow& row ) {
  datacount++;

  if ( NULL != file && data.size( ) >= CACHE_LIMIT ) {
    // copy current data list to disk
    cache( );
  }

  int rowscale = DataRow::scale( row.data, iswave );
  int oldhigh = highval;
  int oldlow = lowval;

  DataRow::hilo( row.data, highval, lowval, rowscale );
  int myscale = scale( );
  if ( rowscale > myscale ) {
    scale( rowscale );

    // see if our high, low vals are really bigger than our new vals
    // given that our scale just changed

    double myhi = static_cast<double> ( oldhigh ) / static_cast<double> ( myscale );
    double mylo = static_cast<double> ( oldlow ) / static_cast<double> ( myscale );

    double rowhi = static_cast<double> ( highval ) / static_cast<double> ( rowscale );
    double rowlo = static_cast<double> ( lowval ) / static_cast<double> ( rowscale );

    if ( myhi > rowhi ) {
      highval = oldhigh;
    }
    if ( mylo < rowlo ) {
      lowval = oldlow;
    }
  }

  if ( !row.extras.empty( ) ) {
    for ( const auto& x : row.extras ) {
      extras( ).push_back( x.first );
    }
  }

  DataRow * lastins = new DataRow( row );
  data.push_back( std::unique_ptr<DataRow>( lastins ) );

  if ( row.time > lastdata ) {
    lastdata = row.time;
  }
  if ( row.time < firstdata ) {
    firstdata = row.time;
  }
  dates.push_front( row.time );
}

void BasicSignalData::setWave( bool wave ) {
  iswave = wave;
}

bool BasicSignalData::wave( ) const {
  return iswave;
}

int BasicSignalData::uncache( int max ) {
  int loop = 0;
  const int BUFFSZ = 4096;
  char buff[BUFFSZ];

  while ( loop < max ) {
    char * justread = std::fgets( buff, BUFFSZ, file );
    if ( NULL == justread ) {
      break;
    }
    std::string read( justread );

    // first things first: if we have attributes, cut them out 
    const size_t barpos = read.find( "|" );
    std::string extras;
    if ( std::string::npos != barpos ) {
      // we have attributes!
      extras = read.substr( barpos + 1 );
      read = read.substr( 0, barpos );
    }

    std::stringstream ss( read );
    dr_time t;
    std::string val;
    std::string high;
    std::string low;

    ss >> t;
    ss >> val;
    ss >> high;
    ss >> low;

    std::map<std::string, std::string> attrs;
    if ( !extras.empty( ) ) {
      // split on |, then split again on "="
      std::string token;
      std::stringstream extrastream( extras );
      while ( std::getline( extrastream, token, '|' ) ) {
        // now split on =
        int eqpos = token.find( "=" );
        std::string key = token.substr( 0, eqpos );
        std::string val = token.substr( eqpos + 1 );

        // strip the \n if it's there
        const size_t newlinepos = val.rfind( "\n" );
        if ( std::string::npos != newlinepos ) {
          val = val.substr( 0, newlinepos );
        }
        attrs[key] = val;
      }
    }


    data.push_back( std::unique_ptr<DataRow>( new DataRow( t, val, high, low, attrs ) ) );

    loop++;
  }

  return loop;
}

const std::deque<dr_time>& BasicSignalData::times( ) const {
  return dates;
}
