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

const int BasicSignalData::CACHE_LIMIT = 30000;

BasicSignalData::BasicSignalData( const std::string& name, bool wavedata )
: label( name ), firstdata( std::numeric_limits<dr_time>::max( ) ), lastdata( 0 ),
datacount( 0 ), popping( false ), iswave( wavedata ), highval( -std::numeric_limits<double>::max( ) ),
lowval( std::numeric_limits<double>::max( ) ), file( nullptr ) {
  setMeta( SignalData::TIMEZONE, "UTC" );
}

BasicSignalData::BasicSignalData( const BasicSignalData& orig ) : SignalData( orig ),
label( orig.label ), firstdata( orig.firstdata ), lastdata( orig.lastdata ),
datacount( orig.datacount ), popping( orig.popping ), iswave( orig.iswave ),
highval( orig.highval ), lowval( orig.lowval ), file( nullptr ) {
  for ( auto const& i : orig.data ) {
    data.push_front( std::unique_ptr<DataRow>( new DataRow( *i ) ) );
  }
  setMeta( SignalData::TIMEZONE, "UTC" );
}

BasicSignalData::~BasicSignalData( ) {
  data.clear( );
  if ( nullptr != file ) {
    std::fclose( file );
  }
}

std::unique_ptr<SignalData> BasicSignalData::shallowcopy( bool includedates ) {
  std::unique_ptr<BasicSignalData> copy( new BasicSignalData( label ) );

  if ( includedates ) {
    copy->firstdata = this->firstdata;
    copy->lastdata = this->lastdata;
  }

  for ( auto x : metad( ) ) {
    copy->setMeta( x.first, x.second );
  }
  for ( auto x : metai( ) ) {
    copy->setMeta( x.first, x.second );
  }
  for ( auto x : metas( ) ) {
    copy->setMeta( x.first, x.second );
  }
  copy->popping = this->popping;
  for ( std::string& s : extras( ) ) {
    copy->extras( ).push_back( s );
  }
  return std::move( copy );
}

dr_time BasicSignalData::startTime( ) const {
  return firstdata;
}

dr_time BasicSignalData::endTime( ) const {
  return lastdata;
}

std::unique_ptr<DataRow> BasicSignalData::pop( ) {
  if ( !popping ) {
    startPopping( );
  }

  if ( nullptr != file && data.empty( ) ) {
    int lines = uncache( );
    if ( 0 == lines ) {
      std::fclose( file );
      file = nullptr;
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
  if ( nullptr != file ) {
    cache( ); // copy any extra rows to disk
    std::rewind( file );
  }
}

double BasicSignalData::highwater( ) const {
  return highval;
}

double BasicSignalData::lowwater( ) const {
  return lowval;
}

void BasicSignalData::cache( ) {
  std::stringstream ss;

  if( nullptr == file ){
    file = tmpfile();
  }

  while ( !data.empty( ) ) {
    std::unique_ptr<DataRow> a = std::move( data.front( ) );
    data.pop_front( );

    ss << a->time << " " << a->data << " " << a->high << " " << a->low << " ";
    if ( !a->extras.empty( ) ) {
      for ( const auto& x : a->extras ) {
        ss << "|" << x.first << "=" << x.second;
      }
    }
    ss << "\n";
  }
  std::fputs( ss.str( ).c_str( ), file );
  fflush( file );
}

void BasicSignalData::add( const DataRow& row ) {

  if ( data.size( ) >= CACHE_LIMIT ) {
    // copy current data list to disk
    cache( );
  }

  datacount++;

  int rowscale = DataRow::scale( row.data, iswave );
  DataRow::hilo( row.data, highval, lowval );
  int myscale = scale( );
  if ( rowscale > myscale ) {
    scale( rowscale );
  }

  if ( !row.extras.empty( ) ) {
    for ( const auto& x : row.extras ) {
      extras( x.first );
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

const std::deque<dr_time> BasicSignalData::times( ) const {
  return std::deque<dr_time>( dates.begin( ), dates.end( ) );
}
