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
#include "SignalData.h"
#include <cmath>
#include <sstream>
#include <vector>
#include <iostream>
#include <limits>

DataRow::DataRow( const dr_time& t, const std::string& d, const std::string& hi,
      const std::string& lo, std::map<std::string, std::string> exts ) : data( d ),
      high( hi ), low( lo ), extras( exts ), time( t ) {
}

DataRow::DataRow( ) : data( "" ), high( "" ), low( "" ), time( 0 ) {
}

DataRow::DataRow( const DataRow& orig )
: data( orig.data ), high( orig.high ), low( orig.low ), extras( orig.extras ), 
time( orig.time ) {
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
    int myscale = 0;
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
    return 0;
  }

  return val.length( ) - pos - 1; // -1 for the .
}

void DataRow::hilo( const std::string& data, double& highval, double& lowval ) {
  std::stringstream stream( data );
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    if ( !( SignalData::MISSING_VALUESTR == each || "32768" == each ) ) {
      double v = std::stod( each );
      if ( v > highval ) {
        highval = v;
      }
      if ( v < lowval ) {
        lowval = v;
      }
    }
  }
}

std::vector<int> DataRow::ints( int scale ) const {
  return ints( data, scale );
}

std::vector<short> DataRow::shorts( int scale ) const {
  return shorts( data, scale );
}

std::vector<int> DataRow::ints( const std::string& data, int scale ) {

  const int scalefactor = std::pow( 10, scale );

  std::stringstream stream( data );
  std::vector<int> vals;
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    if ( SignalData::MISSING_VALUESTR == each ) {
      // don't scale missing numbers
      vals.push_back( SignalData::MISSING_VALUE );
    }
    else if ( 0 == scale ) {
      vals.push_back( std::stoi( each ) );
    }
    else {
//      try{
        vals.push_back( (int) ( std::stof( each ) * scalefactor ) );
//      }
//      catch( std::invalid_argument x){
//        std::cout<<data<<std::endl;
//        std::cout<<"failed on: "<<each<<std::endl;
//      }
    }
  }
  return vals;
}

std::vector<short> DataRow::shorts( const std::string& data, int scale ) {
  const int scalefactor = std::pow( 10, scale );

  std::stringstream stream( data );
  std::vector<short> vals;
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    if ( SignalData::MISSING_VALUESTR == each ) {
      // don't scale missing values
      vals.push_back( SignalData::MISSING_VALUE );
    }
    else if ( 0 == scale ) {
      // FIXME: check against short limits?
      int val = std::stoi( each );
      vals.push_back( (short) val );
    }
    else {
      vals.push_back( (short) ( std::stof( each ) * scalefactor ) );
    }
  }
  return vals;
}
