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

#include "DataSetDataCache.h"
#include "DataRow.h"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>

const int DataSetDataCache::CACHE_LIMIT = 15000;

DataSetDataCache::DataSetDataCache( const std::string& name, bool largefile )
: label( name ), _uom( "Uncalib" ), firstdata( 2099999999 ), lastdata( 0 ),
datacount( 0 ), _scale( 1 ) {
  file = ( largefile ? std::tmpfile( ) : NULL );
}

DataSetDataCache::DataSetDataCache( const DataSetDataCache& orig ) : label( orig.label ),
_uom( orig._uom ), firstdata( orig.firstdata ), lastdata( orig.lastdata ),
datacount( orig.datacount ), _scale( orig._scale ) {
  for ( auto const& i : orig.data ) {
    data.push_front( std::unique_ptr<DataRow>( new DataRow( *i ) ) );
  }
}

const time_t& DataSetDataCache::startTime( ) const {
  return firstdata;
}

const time_t& DataSetDataCache::endTime( ) const {
  return lastdata;
}

std::unique_ptr<DataRow> DataSetDataCache::pop( ) {
  datacount--;
  if ( NULL != file && data.empty( ) ) {
    int lines = uncache( );
    if ( 0 == lines ) {
      std::fclose( file );
      file = NULL;
    }
  }

  std::unique_ptr<DataRow> row = std::move( data.front( ) );
  data.pop_front( );
  return row;
}

DataSetDataCache::~DataSetDataCache( ) {
  data.clear( );
  if ( NULL != file ) {
    std::fclose( file );
  }
}

void DataSetDataCache::setUom( const std::string& u ) {
  _uom = u;
}

int DataSetDataCache::size( ) const {
  return datacount;
}

const std::string& DataSetDataCache::name( ) const {
  return label;
}

const std::string& DataSetDataCache::uom( ) const {
  return _uom;
}

void DataSetDataCache::startPopping( ) {
  if ( NULL != file ) {
    cache( ); // copy any extra rows to disk
    std::rewind( file );
  }
}

void DataSetDataCache::cache( ) {
  while ( !data.empty( ) ) {
    std::unique_ptr<DataRow> a = std::move( data.front( ) );
    data.pop_front( );
    std::string filedata = std::to_string( a->time ) + " " + a->data
        + " " + a->high + " " + a->low + "\n";
    std::fputs( filedata.c_str( ), file );
  }
}

void DataSetDataCache::add( const DataRow& row ) {
  datacount++;

  if ( NULL != file && data.size( ) >= CACHE_LIMIT ) {
    // copy current data list to disk
    cache( );
  }

  data.push_back( std::unique_ptr<DataRow>( new DataRow( row ) ) );

  if ( row.time > lastdata ) {
    lastdata = row.time;
  }
  if ( row.time < firstdata ) {
    firstdata = row.time;
  }
}

int DataSetDataCache::uncache( int max ) {
  int loop = 0;
  const int BUFFSZ = 4096;
  char buff[BUFFSZ];

  while ( loop < max ) {
    char * justread = std::fgets( buff, BUFFSZ, file );
    if ( NULL == justread ) {
      break;
    }
    std::string read( justread );
    std::stringstream ss( read );
    time_t t;

    std::string val;
    std::string high;
    std::string low;

    ss >> t;
    ss >> val;
    ss >> high;
    ss >>low;

    data.push_back( std::unique_ptr<DataRow>( new DataRow( t, val, high, low ) ) );

    loop++;
  }

  return loop;
}

int DataSetDataCache::scale( ) const {
  return _scale;
}

void DataSetDataCache::setScale( int x ) {
  _scale = x;
}