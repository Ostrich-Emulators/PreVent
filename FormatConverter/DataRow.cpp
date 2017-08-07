/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataRow.cpp
 * Author: ryan
 * 
 * Created on August 3, 2016, 7:50 AM
 */

#include "DataRow.h"
#include <cmath>
#include <sstream>
#include <vector>

DataRow::DataRow( const time_t& t, const std::string& d, const std::string& hi,
    const std::string& lo ) : time( t ), data( d ), high( hi ), low( lo ) {
}

DataRow::DataRow( ) : time( 0 ), data( "" ), high( "" ), low( "" ) {
}

DataRow::DataRow( const DataRow& orig )
: time( orig.time ), data( orig.data ), high( orig.high ), low( orig.low ) {
}

DataRow& DataRow::operator=(const DataRow& orig ) {
  if ( &orig != this ) {
    this->high = orig.high;
    this->low = orig.low;
    this->data = orig.data;
    this->time = orig.time;
  }

  return *this;
}

DataRow::~DataRow( ) {
}

void DataRow::clear( ) {
  time = 0;
  high = "";
  low = "";
  data = "";
}

int DataRow::scale( const std::string& val ) {
  // probably dumb, but we pretend the value is a filename, and that
  // makes the extension the number of decimal places in the mantissa
  size_t pos = val.find_last_of( '.', val.length( ) );
  if ( val.npos == pos || ".0" == val.substr( pos ) ) {
    return 1;
  }

  return (int) std::pow( 10, val.length( ) - pos - 1 ); // -1 for the .
}

std::vector<int> DataRow::values( ) const {
  return values( data );
}

std::vector<int> DataRow::values( const std::string& data ) {
  std::stringstream stream( data );
  std::vector<int> vals;
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    vals.push_back( std::stoi( each ) );
  }
  return vals;
}

std::vector<short> DataRow::shorts( const std::string& data ) {
  std::stringstream stream( data );
  std::vector<short> vals;
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    vals.push_back( (short) ( std::stoi( each ) ) );
  }
  return vals;
}
