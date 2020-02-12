
#include "StatisticalSignalData.h"
#include "SignalUtils.h"
#include "DataRow.h"
#include <limits>
#include <iostream>

namespace FormatConverter {

  StatisticalSignalData::StatisticalSignalData( const std::string& name, bool iswave )
  : BasicSignalData( name, iswave ), total( 0 ), count( 0 ), _min( std::numeric_limits<double>::max( ) ),
  _max( std::numeric_limits<double>::min( ) ) {
  }

  StatisticalSignalData::~StatisticalSignalData( ) {
  }

  double StatisticalSignalData::avg( ) const {
    if ( 0 == count ) {
      std::cerr << "no elements to average" << std::endl;
      return 0;
    }
    return total / count;
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
    size_t halfway = ( 0 == count % 2 ? count / 2 : ( count + 1 ) / 2 );
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

  void StatisticalSignalData::add( const FormatConverter::DataRow& row ) {
    count++;
    double val = std::stod( row.data );
    total += val;

    if ( val < _min ) {
      _min = val;
    }
    if ( val > _max ) {
      _max = val;
    }

    if ( 0 == numcounts.count( val ) ) {
      numcounts.insert( std::make_pair( val, 0 ) );
    }
    numcounts[val]++;
  }
}