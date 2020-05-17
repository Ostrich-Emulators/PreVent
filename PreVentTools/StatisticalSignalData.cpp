
#include "StatisticalSignalData.h"
#include "SignalUtils.h"
#include "DataRow.h"
#include <limits>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace FormatConverter{

  StatisticalSignalData::StatisticalSignalData( const std::unique_ptr<SignalData>& data ) : SignalDataWrapper( data ),
      total( 0 ), _count( 0 ), _min( std::numeric_limits<double>::max( ) ),
      _max( std::numeric_limits<double>::min( ) ) { }

  StatisticalSignalData::StatisticalSignalData( SignalData * data ) : SignalDataWrapper( data ),
      total( 0 ), _count( 0 ), _min( std::numeric_limits<double>::max( ) ),
      _max( std::numeric_limits<double>::min( ) ) { }

  StatisticalSignalData::~StatisticalSignalData( ) { }

  double StatisticalSignalData::mean( ) const {
    if ( 0 == _count ) {
      std::cerr << "no elements to average" << std::endl;
      return 0;
    }
    return total / _count;
  }

  double StatisticalSignalData::min( ) const {
    return _min;
  }

  double StatisticalSignalData::max( ) const {
    return _max;
  }

  double StatisticalSignalData::median( ) const {
    // std::maps are sorted on the keys, but we need to figure out
    // the middle number, so iterate until we hit our number
    size_t halfway = ( 0 == _count % 2 ? _count / 2 : ( _count + 1 ) / 2 );
    size_t left = halfway;
    for ( auto& m : numcounts ) {
      if ( m.second > left ) {
        return m.first;
      }
      left -= m.second;
    }

    // we should never get here unless there are no elements in our map
    return 0;
  }

  double StatisticalSignalData::mode( ) const {
    auto modess = modes( );
    if ( modess.empty( ) ) {
      std::cerr << "no elements to count" << std::endl;
      modess.push_back( 0 );
    }

    return modess[0];
  }

  std::vector<double> StatisticalSignalData::modes( ) const {
    std::vector<double> vals;
    size_t counter = 0;

    for ( auto mv : numcounts ) {
      if ( mv.second > counter ) {
        counter = mv.second;
        vals.clear( );
        vals.push_back( mv.first );
      }
      else if ( mv.second == counter ) {
        vals.push_back( mv.first );
      }
    }

    return vals;
  }

  double StatisticalSignalData::variance( ) const {
    double var = 0;
    double avg = mean( );
    for ( const auto& m : numcounts ) {
      double diff = m.first - avg;
      var += ( diff * diff ) * m.second;
    }

    return var / ( _count - 1 );
  }

  size_t StatisticalSignalData::count( ) const {
    return _count;
  }

  double StatisticalSignalData::stddev( ) const {
    return std::sqrt( variance( ) );
  }

  bool StatisticalSignalData::add( const FormatConverter::DataRow& row ) {
    SignalDataWrapper::add( row );

    std::vector<double> values( row.doubles( ) );

    for ( auto val : values ) {
      if ( val != SignalData::MISSING_VALUE ) {
        total += val;

        if ( val < _min ) {
          _min = val;
        }
        if ( val > _max ) {
          _max = val;
        }

        // boy, we REALLY hope we have a small(ish) domain here...
        if ( 0 == numcounts.count( val ) ) {
          numcounts.insert( std::make_pair( val, 0 ) );
        }
        numcounts[val]++;
      }
    }

    _count += values.size( );
    return true;
  }
}