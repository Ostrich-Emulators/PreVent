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

#include "SignalData.h"
#include "DataRow.h"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <limits>
#include <queue>

const int SignalData::CACHE_LIMIT = 15000;

const std::string SignalData::HERTZ = "Sample Frequency (Hz)";
const std::string SignalData::SCALE = "Scale";
const std::string SignalData::UOM = "Unit of Measure";
const std::string SignalData::MSM = "Missing Value Marker";
const std::string SignalData::TIMEZONE = "Timezone";
const std::string SignalData::VALS_PER_DR = "Readings Per Time";
const int SignalData::MISSING_VALUE = -32768;


SignalData::SignalData( const std::string& name, bool largefile, bool wavedata )
: label( name ), firstdata( std::numeric_limits<time_t>::max( ) ), lastdata( 0 ),
datacount( 0 ), popping( false ), iswave( wavedata ) {
  file = ( largefile ? std::tmpfile( ) : NULL );
  setScale( 1 );
  setValuesPerDataRow( 1 );
  setUom( "Uncalib" );
}

SignalData::SignalData( const SignalData& orig ) : label( orig.label ),
firstdata( orig.firstdata ), lastdata( orig.lastdata ),
datacount( orig.datacount ), metadatas( orig.metadatas ),
metadatai( orig.metadatai ), metadatad( orig.metadatad ), popping( orig.popping ),
iswave( orig.iswave ) {
  for ( auto const& i : orig.data ) {
    data.push_front( std::unique_ptr<DataRow>( new DataRow( *i ) ) );
  }
}

std::unique_ptr<SignalData> SignalData::shallowcopy( bool includedates ) {
  std::unique_ptr<SignalData> copy( new SignalData( label, ( NULL != file ) ) );

  if ( includedates ) {
    copy->firstdata = this->firstdata;
    copy->lastdata = this->lastdata;
  }

  copy->metad( ).insert( metadatad.begin( ), metadatad.end( ) );
  copy->metai( ).insert( metadatai.begin( ), metadatai.end( ) );
  copy->metas( ).insert( metadatas.begin( ), metadatas.end( ) );
  copy->popping = this->popping;
  return std::move( copy );
}

std::map<std::string, std::string>& SignalData::metas( ) {
  return metadatas;
}

std::map<std::string, int>& SignalData::metai( ) {
  return metadatai;
}

std::map<std::string, double>& SignalData::metad( ) {
  return metadatad;
}

const std::map<std::string, std::string>& SignalData::metas( ) const {
  return metadatas;
}

const std::map<std::string, int>& SignalData::metai( ) const {
  return metadatai;
}

const std::map<std::string, double>& SignalData::metad( ) const {
  return metadatad;
}

double SignalData::hz( ) const {
  return ( 0 == metadatad.count( SignalData::HERTZ )
        ? 1
        : metadatad.at( SignalData::HERTZ ) );
}

const time_t& SignalData::startTime( ) const {
  return firstdata;
}

const time_t& SignalData::endTime( ) const {
  return lastdata;
}

bool SignalData::empty( ) const {
  return ( 0 == size( ) );
}

std::unique_ptr<DataRow> SignalData::pop( ) {
  if ( !popping ) {
    startPopping( );
  }

  datacount--;
  dates.pop_back( );
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

SignalData::~SignalData( ) {
  data.clear( );
  if ( NULL != file ) {
    std::fclose( file );
  }
}

void SignalData::setUom( const std::string& u ) {
  metadatas[UOM] = u;
}

size_t SignalData::size( ) const {
  return datacount;
}

const std::string& SignalData::name( ) const {
  return label;
}

const std::string& SignalData::uom( ) const {
  return metadatas.at( UOM );
}

void SignalData::startPopping( ) {
  popping = true;
  if ( NULL != file ) {
    cache( ); // copy any extra rows to disk
    std::rewind( file );
  }
}

void SignalData::cache( ) {
  while ( !data.empty( ) ) {
    std::unique_ptr<DataRow> a = std::move( data.front( ) );
    data.pop_front( );
    std::string filedata = std::to_string( a->time ) + " " + a->data
          + " " + a->high + " " + a->low + "\n";
    std::fputs( filedata.c_str( ), file );
  }
}

void SignalData::add( const DataRow& row ) {
  datacount++;

  if ( NULL != file && data.size( ) >= CACHE_LIMIT ) {
    // copy current data list to disk
    cache( );
  }

  int rowscale = DataRow::scale( row.data );
  if ( rowscale > scale( ) ) {
    setScale( rowscale );
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

void SignalData::setWave( bool wave ) {
  iswave = wave;
}

bool SignalData::wave( ) const {
  return iswave;
}

int SignalData::uncache( int max ) {
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

int SignalData::scale( ) const {
  return metadatai.at( SCALE );
}

void SignalData::setScale( int x ) {
  metadatai[SCALE] = x;
}

const std::deque<time_t>& SignalData::times( ) const {
  return dates;
}

void SignalData::setValuesPerDataRow(int x){
  metadatai[VALS_PER_DR] = x;
}

int SignalData::valuesPerDataRow() const {
  return metadatai.at( VALS_PER_DR );
}
