/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalUtils.h"

#include "SignalData.h"
#include "DataRow.h"
#include "StpXmlReader.h"
#include <limits>
#include <vector>

SignalUtils::SignalUtils( ) {
}

SignalUtils::SignalUtils( const SignalUtils& ) {
}

SignalUtils::~SignalUtils( ) {
}

time_t SignalUtils::firstlast( const std::map<std::string, std::unique_ptr<SignalData>>&map,
    time_t * first, time_t * last ) {

  time_t earliest = std::numeric_limits<time_t>::max( );

  if ( nullptr != first ) {
    *first = std::numeric_limits<time_t>::max( );
  }
  if ( nullptr != last ) {
    *last = 0;
  }
  for ( const auto& m : map ) {
    if ( nullptr != first ) {
      if ( m.second->startTime( ) < *first ) {
        *first = m.second->startTime( );
        earliest = m.second->startTime( );
      }
    }

    if ( nullptr != last ) {
      if ( m.second->endTime( ) > *last ) {
        *last = m.second->endTime( );
      }
    }
  }

  return earliest;
}

std::map<std::string, std::unique_ptr<SignalData>> SignalUtils::sync(
    std::map<std::string, std::unique_ptr<SignalData> >& data ) {

  std::map<std::string, std::unique_ptr < SignalData>> ret;

  time_t earliest;
  time_t latest;
  firstlast( data, &earliest, &latest );

  float freq = data.begin( )->second->metad( ).at( SignalData::HERTZ );
  int timestep = ( freq < 1 ? 1 / freq : 1 );

  // start a-poppin'!
  for ( const auto& m : data ) {
    std::string name( m.first );
    ret[m.first] = std::move( m.second->shallowcopy( ) );

    m.second->startPopping( );
    time_t lasttime = earliest;

    while ( 0 != m.second->size( ) ) {
      const auto& row = m.second->pop( );

      // fill in any rows between the last time and this signal's current time
      for ( int thistime = lasttime; thistime < row->time; thistime += timestep ) {
        ret[name]->add( DataRow( thistime, StpXmlReader::MISSING_VALUESTR ) );
      }

      ret[name]->add( *row );
      lasttime = row->time;
    }

    // fill in any rows after our signal's last time, but before the set's end time
    for ( int thistime = lasttime; thistime < latest; thistime += timestep ) {
      ret[name]->add( DataRow( thistime, StpXmlReader::MISSING_VALUESTR ) );
    }
  }

  return ret;
}
