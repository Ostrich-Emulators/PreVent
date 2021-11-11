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
#include "SignalUtils.h"
#include "Log.h"
#include <cmath>
#include <math.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <limits>
#include <charconv>
#include <iomanip>

namespace FormatConverter{

  TimedData::TimedData( dr_time t, const std::string& v ) : time( t ), data( v ) { }

  TimedData::TimedData( const TimedData& orig ) : time( orig.time ), data( orig.data ) { }

  TimedData& TimedData::operator=(const TimedData& orig ) {
    if ( &orig != this ) {
      this->time = orig.time;
      this->data = orig.data;
    }
    return *this;
  }

  TimedData::~TimedData( ) { }

  const std::set<std::string> DataRow::hiloskips = { "-32768", "32768" };

  void DataRow::intify( const std::string_view& _strval, size_t dotpos, int * val, int * scale ) {
    //Log::info( ) << _strval << "\t" << dotpos << "\t" << *val << "\t" << *scale << std::endl;
    if ( std::string::npos == dotpos ) {
      *scale = 0;
      std::from_chars( _strval.data( ), _strval.data( ) + _strval.size( ), *val );
    }
    else {
      bool isneg = ( '-' == _strval[0] );

      // -0.5 will show up as 0 (whole), 5 (fraction), so we need to know if
      // we have a negative sign up front. If we do, ignore it while converting
      // to an (always positive) int, but then multiply by -1 at the end;

      auto strval = ( isneg
          ? _strval.substr( 1 )
          : _strval );
      if ( isneg ) {
        // if we removed the -, the decimal point moved, too!
        dotpos--;
      }

      int whole = 0;
      int fraction = 0;
      std::from_chars( strval.data( ), strval.data( ) + dotpos, whole );
      std::from_chars( strval.data( ) + dotpos + 1, strval.data( ) - dotpos, fraction );

      if ( 0 == fraction ) {
        *scale = 0;
        *val = whole;
      }
      else {
        *scale = static_cast<int> ( strval.length( ) - dotpos - 1 );
        *val = fraction;

        if ( 0 != whole ) {
          *val += whole * std::pow( 10, *scale );
        }
      }

      if ( isneg ) {
        *val *= -1;
//        Log::info( ) << _strval << " converted to " << *val << std::endl;
//        Log::info( ) << "scale:" << *scale << " (strval.len: " << _strval.length( ) << "; dotpos: " << dotpos << ")" << std::endl;
      }
    }
  }

  void DataRow::intify( const std::string_view& data, std::vector<int>& vals, int * datascale ) {
    int maxscale = 0;

    for ( const auto& smallview : SignalUtils::splitcsv( data ) ) {
      // might still be an integer if we have something like .000
      int val;
      int scale;
      intify( smallview, smallview.find( '.' ), &val, &scale );
      if ( scale > maxscale ) {
        // fix all values in the vector before adding this one
        const int upgrade = std::pow( 10, scale - maxscale );
        for ( auto& f : vals ) {
          if ( SignalData::MISSING_VALUE != f ) {
            f *= upgrade;
          }
        }
        maxscale = scale;
      }
      else if ( scale < maxscale ) {
        // fix this value to match maxscale
        if ( val != SignalData::MISSING_VALUE ) {
          val *= std::pow( 10, maxscale - scale );
        }
      }

      vals.push_back( val );
    }

    *datascale = maxscale;
  }

  std::unique_ptr<DataRow> DataRow::from( const dr_time& time, const std::string& data ) {
    return ( std::string::npos == data.find( ',' )
        ? one( time, data )
        : many( time, data ) );
  }

  std::unique_ptr<DataRow> DataRow::one( const dr_time& time, const std::string& data ) {
    if ( 0 != hiloskips.count( data ) ) {
      return std::make_unique<DataRow>( time, SignalData::MISSING_VALUE );
    }

    size_t dotpos = data.find( '.' );
    if ( std::string::npos == dotpos ) {
      return std::make_unique<DataRow>( time, std::stoi( data ) );
    }
    else {
      int val;
      int scale;
      intify( data, dotpos, &val, &scale );
      return std::make_unique<DataRow>( time, val, scale );
    }
  }

  std::unique_ptr<DataRow> DataRow::many( const dr_time& time, const std::string & data ) {
    std::vector<int> vals;
    int maxscale;
    intify( std::string_view( data ), vals, &maxscale );
    return std::make_unique<DataRow>( time, vals, maxscale );
  }

  DataRow::DataRow( const dr_time& _time, const std::vector<int>& _data, int _scale,
      std::map<std::string, std::string> _extras ) : time( _time ), data( _data.begin( ), _data.end( ) ),
      scale( _scale ), extras( _extras ) { }

  DataRow::DataRow( const dr_time& _time, int _data, int _scale,
      std::map<std::string, std::string> _extras ) : time( _time ), scale( _scale ),
      extras( _extras ) {
    data.push_back( _data );
  }

  DataRow::DataRow( const DataRow & orig ) : time( orig.time ), data( orig.data.begin( ), orig.data.end( ) ),
      scale( orig.scale ), extras( orig.extras ) { }

  DataRow::DataRow( const dr_time& _time, const std::vector<double>& doubles,
      std::map<std::string, std::string> _extras, int maxprecision )
      : time( _time ), scale( 0 ), extras( _extras ) {

    data.reserve( doubles.size( ) );

    // if we have all ints, there's no reason to handle fractions
    bool seenfloat = false;
    double intpart;
    for ( auto d : doubles ) {
      double fraction = std::modf( d, &intpart );
      if ( fraction != 0 ) {
        seenfloat = true;
        break;
      }
    }

    if ( seenfloat ) {
      std::stringstream vals;
      vals << std::setprecision( maxprecision ) << std::fixed;

      if ( SignalData::MISSING_VALUE == doubles[0] || isnan( doubles[0] ) ) {
        vals << SignalData::MISSING_VALUESTR;
      }
      else {
        vals << doubles[0];
      }

      for ( size_t i = 1; i < doubles.size( ); i++ ) {
        vals << ",";
        if ( SignalData::MISSING_VALUE == doubles[i] || isnan( doubles[i] ) ) {
          vals << SignalData::MISSING_VALUESTR;
        }
        else {
          vals << doubles[i];
        }
      }

      intify( vals.str( ), data, &scale );
    }
    else {
      data.insert( data.end( ), doubles.begin( ), doubles.end( ) );
    }
  }

  DataRow & DataRow::operator=(const DataRow & orig ) {
    if ( &orig != this ) {
      this->data = orig.data;
      this->time = orig.time;
      this->extras = orig.extras;
      this->scale = orig.scale;
    }

    return *this;
  }

  DataRow::~DataRow( ) { }

  void DataRow::clear( ) {
    time = 0;
    scale = 0;
    data.clear( );
    extras.clear( );
  }

  void DataRow::rescale( int newscale ) {
    if ( newscale != scale ) {
      int pow10 = std::pow( 10, newscale - scale );
      for ( auto& val : data ) {
        if ( val != SignalData::MISSING_VALUE ) {
          val *= pow10;
        }
      }
    }
  }

  const std::vector<int>& DataRow::ints( ) const {
    return data;
  }

  std::vector<short> DataRow::shorts( ) const {
    return std::vector<short>( data.begin( ), data.end( ) );
  }

  std::vector<double> DataRow::doubles( ) const {
    std::vector<double> vec;
    vec.reserve( data.size( ) );

    const double pow10 = std::pow( 10, scale );
    for ( const auto& v : data ) {
      vec.push_back( SignalData::MISSING_VALUE == v
          ? SignalData::MISSING_VALUE
          : v / pow10 );
    }
    return vec;
  }

  int DataRow::scaleOf( const std::string& value ) {
    const size_t dotpos = value.find( '.' );
    return (std::string::npos == dotpos
        ? 0
        : value.size( ) - dotpos );
  }
}