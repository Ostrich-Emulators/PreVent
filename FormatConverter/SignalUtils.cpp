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
    if ( m.second->startTime( ) < earliest ) {
      earliest = m.second->startTime( );
      if ( nullptr != first ) {
        *first = m.second->startTime( );
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

std::vector<std::vector<std::string>> SignalUtils::syncDatas( std::map<std::string,
    std::unique_ptr<SignalData> >&map ) {

  std::map<std::string, std::unique_ptr<SignalData> > * working = &map;
  std::map<std::string, std::unique_ptr<SignalData> > tmp;

  // figure out if the data is already synced
  time_t earliest;
  time_t latest;
  firstlast( map, &earliest, &latest );
  for ( const auto& m : map ) {
    if ( m.second->startTime( ) != earliest ||
        m.second->endTime( ) != latest ) {
      tmp = sync( map );
      working = &tmp;
      break;
    }
  }

  for ( const auto& m : *working ) {
    m.second->startPopping( );
  }

  std::vector<std::vector < std::string>> rows;
  int rowcnt = working->begin( )->second->size( );
  rows.reserve( rowcnt );

  for ( int rownum = 0; rownum < rowcnt; rownum++ ) {
    //std::cout << "ROW " << rownum << std::endl;

    std::vector<std::string> rowcols;
    rowcols.reserve( working->size( ) );

    for ( auto& m : *working ) {
      //std::cout << "  " << m.first;
      if ( m.second->empty( ) ) {
        std::cout << m.first << " BUG! ran out of data before anyone else" << std::endl;
      }
      else {
        const std::unique_ptr<DataRow>& row = m.second->pop( );
        // std::cout << " row: " << row->time << " " << row->data << std::endl;
        rowcols.push_back( row->data );
      }
    }
    rows.push_back( rowcols );
  }

  return rows;
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
  for ( auto& m : data ) {
    std::string name( m.first );
    //std::cout << "syncing " << name << std::endl;
    ret[m.first] = std::move( m.second->shallowcopy( ) );

    m.second->startPopping( );
    time_t nexttime = earliest;

    while ( 0 != m.second->size( ) ) {
      auto row = std::move( m.second->pop( ) );
      //std::cout << "  popped row at " << row->time << "; "
      //    << m.second->size( ) << " rows left" << std::endl;

      // fill in any rows between the last time and this signal's current time
      fillGap( ret[name], row, nexttime, timestep );

      ret[name]->add( *row );
      nexttime = row->time + timestep;
    }

    // fill in any rows after our signal's last time, but before the set's end time
    for ( nexttime; nexttime < latest + timestep; nexttime += timestep ) {
      //std::cout << "    filling to end row at time " << nexttime << std::endl;
      ret[name]->add( DataRow( nexttime, StpXmlReader::MISSING_VALUESTR ) );
    }
  }

  //  for ( auto& x : ret ) {
  //    std::cout << x.first << " " << x.second->size( ) << " rows" << std::endl;
  //  }

  return ret;
}

void SignalUtils::fillGap( std::unique_ptr<SignalData>& data, std::unique_ptr<DataRow>& row,
    time_t& nexttime, const int& timestep ) {
  if ( row->time == nexttime ) {
    return;
  }

  // STP time sometimes drifts forward (and backward?)
  // so if our timestep is 2s, and we get 3s, just move it back 1s
  // HOWEVER: we might get this situation where we have a gap AND a drift
  // concurrently. In this situation, make the best guess we can

  // easy case: 1 second time drift
  if ( 2 == timestep && row->time == nexttime + 1 ) {
    std::cout << data->name( ) << " correcting time drift "
        << row->time << " to " << nexttime << std::endl;
    row->time = nexttime;
    return;
  }

  for ( nexttime; nexttime < row->time - timestep; nexttime += timestep ) {
    std::cout << data->name( ) << " adding filler row at time " << nexttime << std::endl;
    data->add( DataRow( nexttime, StpXmlReader::MISSING_VALUESTR ) );
  }

  if ( 2 == timestep && row->time == nexttime + 1 ) {
    std::cout << data->name( ) << " correcting time drift after filling data "
        << row->time << " to " << ( row->time - 1 ) << std::endl;
    row->time -= 1;
  }
  else {
    data->add( DataRow( nexttime, StpXmlReader::MISSING_VALUESTR ) );
  }
}

