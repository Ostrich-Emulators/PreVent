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
#include "Reader.h"
#include <cmath>
#include <sstream>
#include <vector>
#include <iostream>

DataRow::DataRow( const dr_time& t, const std::string& d, const std::string& hi,
      const std::string& lo, std::map<std::string, std::string> exts ) : time( t ),
data( d ), high( hi ), low( lo ), extras( exts ) {
}

DataRow::DataRow( ) : time( 0 ), data( "" ), high( "" ), low( "" ) {
}

DataRow::DataRow( const DataRow& orig )
: time( orig.time ), data( orig.data ), high( orig.high ), low( orig.low ),
extras( orig.extras ) {
}

DataRow& DataRow::operator=(const DataRow& orig ) {
  if ( &orig != this ) {
    this->high = orig.high;
    this->low = orig.low;
    this->data = orig.data;
    this->time = orig.time;
    this->extras = orig.extras;
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
  extras.clear( );
}

int DataRow::scale( const std::string& val, bool iswave ) {
  if ( iswave ) {
    int myscale = 1;
    std::stringstream ss( val );
    for ( std::string each; std::getline( ss, each, ',' ); ) {
      int newscale = scale( each, false );
      if ( newscale > myscale ) {
        myscale = newscale;
      }
    }
    return myscale;
  }

  // probably dumb, but we pretend the value is a filename, and that
  // makes the extension the number of decimal places in the mantissa
  size_t pos = val.find_last_of( '.', val.length( ) );
  if ( val.npos == pos || ".0" == val.substr( pos ) ) {
    return 1;
  }

  return (int) std::pow( 10, val.length( ) - pos - 1 ); // -1 for the .
}

std::vector<int> DataRow::ints( ) const {
  return ints( data );
}

std::vector<int> DataRow::ints( const std::string& data ) {
  std::stringstream stream( data );
  std::vector<int> vals;
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    vals.push_back( std::stoi( each ) );
  }
  return vals;
}

std::vector<short> DataRow::shorts( const std::string& data, int scale ) {
  std::stringstream stream( data );
  std::vector<short> vals;
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    if( Reader::MISSING_VALUESTR == each ){
      // don't scale missing values
      vals.push_back( (short) ( std::stoi(Reader::MISSING_VALUESTR ) ) );
    }
    else{
      vals.push_back( (short) ( std::stof( each ) * scale ) );
    }
  }
  return vals;
}
