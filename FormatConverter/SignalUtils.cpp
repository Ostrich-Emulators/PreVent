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

time_t SignalUtils::firstlast( const std::vector<std::unique_ptr<SignalData>>&signals,
    time_t * first, time_t * last ) {

  time_t earliest = std::numeric_limits<time_t>::max( );

  if ( nullptr != first ) {
    *first = std::numeric_limits<time_t>::max( );
  }
  if ( nullptr != last ) {
    *last = 0;
  }
  for ( const auto& signal : signals ) {
    if ( signal->startTime( ) < earliest ) {
      earliest = signal->startTime( );
      if ( nullptr != first ) {
        *first = signal->startTime( );
      }
    }

    if ( nullptr != last ) {
      if ( signal->endTime( ) > *last ) {
        *last = signal->endTime( );
      }
    }
  }

  return earliest;
}

std::vector<std::vector<std::string>> SignalUtils::syncDatas( std::vector<std::unique_ptr<SignalData> >&signals ) {

  std::vector<std::unique_ptr<SignalData> > * working = &signals;
  std::vector<std::unique_ptr<SignalData> > tmp;

  // figure out if the data is already synced
  time_t earliest;
  time_t latest;
  firstlast( signals, &earliest, &latest );
  for ( const auto& m : signals ) {
    if ( m->startTime( ) != earliest ||
        m->endTime( ) != latest ) {
      tmp = sync( signals );
      working = &tmp;
      break;
    }
  }

  std::vector<std::vector < std::string>> rows;
  int rowcnt = ( *working->begin( ) )->size( );
  rows.reserve( rowcnt );

  for ( int rownum = 0; rownum < rowcnt; rownum++ ) {
    //std::cout << "ROW " << rownum << std::endl;

    std::vector<std::string> rowcols;
    rowcols.reserve( working->size( ) );

    for ( auto& m : *working ) {
      //std::cout << "  " << m.first;
      if ( m->empty( ) ) {
        std::cout << m->name( ) << " BUG! ran out of data before anyone else" << std::endl;

        std::string dummy = StpXmlReader::MISSING_VALUESTR;
        for ( int i = 1; i < m->hz( ); i++ ) {
          dummy.append( "," ).append( StpXmlReader::MISSING_VALUESTR );
        }
        rowcols.push_back( dummy );
      }
      else {
        const std::unique_ptr<DataRow>& row = m->pop( );
        // std::cout << " row: " << row->time << " " << row->data << std::endl;
        rowcols.push_back( row->data );
      }
    }
    rows.push_back( rowcols );
  }

  return rows;
}

std::map<std::string, std::unique_ptr<SignalData>> SignalUtils::mapify(
    std::vector<std::unique_ptr<SignalData>>&data ) {
  std::map<std::string, std::unique_ptr < SignalData>> map;
  for ( auto& s : data ) {
    map[s->name( )] = std::move( s );
  }

  return map;

}

std::vector<std::unique_ptr<SignalData>> SignalUtils::vectorize(
    std::map<std::string, std::unique_ptr<SignalData>>&data ) {
  std::vector<std::unique_ptr < SignalData>> vec;
  for ( auto& m : data ) {
    vec.push_back( std::move( m.second ) );
  }

  return vec;
}

std::vector<std::unique_ptr<SignalData>> SignalUtils::sync(
    std::vector<std::unique_ptr<SignalData> >& data ) {

  std::vector<std::unique_ptr < SignalData>> ret;

  time_t earliest;
  time_t latest;
  firstlast( data, &earliest, &latest );

  float freq = ( *data.begin( ) )->hz( );
  int timestep = ( freq < 1 ? 1 / freq : 1 );

  // start a-poppin'!
  for ( auto& signal : data ) {
    std::unique_ptr<SignalData> copy = std::move( signal->shallowcopy( ) );

    // const std::string& name = signal->name( );
    //std::cout << "syncing " << name << std::endl;

    time_t nexttime = earliest;

    while ( 0 != signal->size( ) ) {
      auto row = std::move( signal->pop( ) );
      //std::cout << "  popped row at " << row->time << "; "
      //    << signal->size( ) << " rows left" << std::endl;

      // fill in any rows between the last time and this signal's current time
      fillGap( copy, row, nexttime, timestep );

      copy->add( *row );
      nexttime = row->time + timestep;
    }

    // fill in any rows after our signal's last time, but before the set's end time
    for ( nexttime; nexttime < latest + timestep; nexttime += timestep ) {
      //std::cout << "    filling to end row at time " << nexttime << std::endl;
      copy->add( DataRow( nexttime, StpXmlReader::MISSING_VALUESTR ) );
    }

    ret.push_back( std::move( copy ) );
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

