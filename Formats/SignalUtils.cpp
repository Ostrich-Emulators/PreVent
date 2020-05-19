/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalUtils.h"

#include "SignalData.h"
#include "DataRow.h"
#include "Reader.h"

#include <set>
#include <limits>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace FormatConverter{

  SignalUtils::SignalUtils( ) { }

  SignalUtils::SignalUtils( const SignalUtils& ) { }

  SignalUtils::~SignalUtils( ) { }

  std::string SignalUtils::trim( std::string & totrim ) {
    // ltrim
    totrim.erase( totrim.begin( ), std::find_if( totrim.begin( ), totrim.end( ),
        std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );

    // rtrim
    totrim.erase( std::find_if( totrim.rbegin( ), totrim.rend( ),
        std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), totrim.end( ) );

    return totrim;
  }

  dr_time SignalUtils::firstlast( const std::map<std::string, std::unique_ptr<SignalData>>&map,
      dr_time * first, dr_time * last ) {

    dr_time earliest = std::numeric_limits<dr_time>::max( );

    if ( nullptr != first ) {
      *first = std::numeric_limits<dr_time>::max( );
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

  dr_time SignalUtils::firstlast( const std::vector<std::unique_ptr<SignalData>>&signals,
      dr_time * first, dr_time * last ) {

    dr_time earliest = std::numeric_limits<dr_time>::max( );

    if ( nullptr != first ) {
      *first = std::numeric_limits<dr_time>::max( );
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

  std::vector<std::vector<int>> SignalUtils::syncDatas( std::vector<std::unique_ptr<SignalData> >&signals ) {

    std::vector<std::unique_ptr<SignalData> > * working = &signals;
    std::vector<std::unique_ptr<SignalData> > tmp;

    // figure out if the data is already synced
    dr_time earliest;
    dr_time latest;
    const size_t size = signals[0]->size( );
    firstlast( signals, &earliest, &latest );
    for ( const auto& m : signals ) {
      // check that every signal has the same start time, end time, and # datapoints
      // if not, we need to sync them
      if ( !( m->startTime( ) == earliest
          && m->endTime( ) == latest
          && m->size( ) == size ) ) {
        //std::cout << "syncing signals" << std::endl;
        tmp = sync( signals );
        working = &tmp;
        break;
      }
    }

    std::vector<std::vector<int>> rows;
    int rowcnt = ( *working->begin( ) )->size( );
    rows.reserve( rowcnt );

    for ( int rownum = 0; rownum < rowcnt; rownum++ ) {
      //std::cout << "ROW " << rownum << std::endl;

      std::vector<int> rowcols( working->size( ) );

      for ( auto& m : *working ) {
        //std::cout << "  " << m.first;
        if ( m->empty( ) ) {
          std::cout << m->name( ) << " BUG! ran out of data before anyone else" << std::endl;

          auto dummy = dummyfill( m, 0 );
          rowcols.insert( rowcols.end( ), dummy->data.begin( ), dummy->data.end( ) );
        }
        else {
          const auto& row = m->pop( );
          // std::cout << " row: " << row->time << " " << row->data << std::endl;
          rowcols.insert( rowcols.end( ), row->data.begin( ), row->data.end( ) );
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

    dr_time earliest;
    dr_time latest;
    firstlast( data, &earliest, &latest );

    //  std::cout << "f/l:\t" << earliest << "\t" << latest << std::endl;
    //  for ( const auto& s : data ) {
    //    std::cout << s->name( ) << "\t" << s->startTime( ) << "\t"
    //        << s->endTime( ) << "\t" << s->size( ) << std::endl;
    //  }

    std::unique_ptr<FormatConverter::DataRow> currenttimes[data.size( )];
    for ( size_t i = 0; i < data.size( ); i++ ) {
      ret.push_back( data[i]->shallowcopy( ) );

      // load the first row for each signal into our current array
      currenttimes[i] = std::move( data[i]->pop( ) );
    }

    std::set<int> empties;
    while ( empties.size( ) < data.size( ) ) {

      // see if any of our current times match our "earliest" time
      // if they do, add it to the SignalData and replace that time with
      // a new datarow
      for ( size_t i = 0; i < data.size( ); i++ ) {
        if ( currenttimes[i] ) {
          // we have a time to check
          if ( currenttimes[i]->time == earliest ) {
            ret[i]->add( std::move( currenttimes[i] ) );

            if ( data[i]->empty( ) ) {
              empties.insert( i );
            }
            else {
              currenttimes[i] = std::move( data[i]->pop( ) );
            }
          }
          else if ( currenttimes[i]->time > earliest ) {
            // don't have a datapoint for this time, so make a dummy one
            ret[i]->add( std::move( dummyfill( data[i], earliest ) ) );
          }
        }
        else {
          // ran out of times fo this signal, so make dummy data for this time
          ret[i]->add( std::move( dummyfill( data[i], earliest ) ) );
        }
      }

      // go through again and figure out which is our new earliest time
      earliest = std::numeric_limits<dr_time>::max( );
      for ( size_t i = 0; i < data.size( ); i++ ) {
        if ( currenttimes[i] && currenttimes[i]->time < earliest ) {

          earliest = currenttimes[i]->time;
        }
      }
    }

    //  for ( auto& x : ret ) {
    //    std::cout << x.first << " " << x.second->size( ) << " rows" << std::endl;
    //  }

    return ret;
  }

  void SignalUtils::fillGap( std::unique_ptr<SignalData>& signal, std::unique_ptr<FormatConverter::DataRow>& row,
      dr_time& nexttime, const int& timestep ) {
    if ( row->time == nexttime ) {
      return;
    }

    // STP time sometimes drifts forward (and backward?)
    // so if our timestep is 2s, and we get 3s, just move it back 1s
    // HOWEVER: we might get this situation where we have a gap AND a drift
    // concurrently. In this situation, make the best guess we can

    // easy case: 1 second time drift
    if ( 2 == timestep && row->time == nexttime + 1 ) {
      std::cout << signal->name( ) << " correcting time drift "
          << row->time << " to " << nexttime << std::endl;
      row->time = nexttime;
      return;
    }

    dr_time fillstart = nexttime;
    for (; nexttime < row->time - timestep; nexttime += timestep ) {
      signal->add( std::move( dummyfill( signal, nexttime ) ) );
    }

    if ( nexttime != fillstart ) {
      std::cout << signal->name( ) << " added filler rows from " << fillstart
          << " to " << nexttime << std::endl;
    }

    if ( 2 == timestep && row->time == nexttime + 1 ) {
      std::cout << signal->name( ) << " correcting time drift after filling data "
          << row->time << " to " << ( row->time - 1 ) << std::endl;
      row->time -= 1;
    }
    else {
      std::cout << signal->name( ) << " added filler row at " << nexttime << std::endl;
      signal->add( std::move( dummyfill( signal, nexttime ) ) );
    }
  }

  std::unique_ptr<DataRow> SignalUtils::dummyfill( std::unique_ptr<SignalData>& signal, const dr_time& time ) {
    return std::make_unique<DataRow>( time, std::vector<int>( signal->hz( ), SignalData::MISSING_VALUE ) );
  }

  std::vector<dr_time> SignalUtils::alltimes( const SignalSet& ss ) {
    std::set<dr_time> times;
    for ( const auto& signal : ss.vitals( ) ) {
      times.insert( signal.get( )->times( ).begin( ), signal.get( )->times( ).end( ) );
    }
    for ( const auto& signal : ss.waves( ) ) {
      times.insert( signal.get( )->times( ).begin( ), signal.get( )->times( ).end( ) );
    }

    std::vector<dr_time> vec( times.begin( ), times.end( ) );
    std::sort( vec.begin( ), vec.end( ) );

    return vec;
  }

  std::vector<size_t> SignalUtils::index( const std::vector<dr_time>& alltimes,
      SignalData& signal ) {

    std::cout << "signaltimes values: " << signal.times( ).size( ) << std::endl;
    std::deque<dr_time> signaltimes( signal.times( ).begin( ), signal.times( ).end( ) );
    std::sort( signaltimes.begin( ), signaltimes.end( ) );
    const double hz = signal.hz( );
    const int rowsPerTime = ( hz < 1.0 ? 1 : (int) hz );
    std::cout << "signaltimes values (2): " << signaltimes.size( ) << std::endl;

    std::vector<size_t> indexes;

    size_t currentIndex = 0;
    for ( dr_time all : alltimes ) {
      std::cout << "all: " << all << "\t front: " << signaltimes.front( ) << std::endl;
      indexes.push_back( currentIndex );
      if ( !signaltimes.empty( ) && signaltimes.front( ) == all ) {

        currentIndex += rowsPerTime;
        signaltimes.pop_front( );
      }

      // else skip ahead until we find the time from the
    }

    return indexes;
  }

  std::string SignalUtils::tosmallstring( double val, int scale ) {
    if ( SignalData::MISSING_VALUE == static_cast<int> ( val ) ) {
      return SignalData::MISSING_VALUESTR;
    }
    else if ( 0 == scale ) {
      return std::to_string( static_cast<int> ( val ) );
    }

    std::string valstr = std::to_string( val );
    auto lastNotZeroPosition = valstr.find_last_not_of( '0' );
    if ( lastNotZeroPosition != std::string::npos && lastNotZeroPosition + 1 < valstr.size( ) ) {
      //We leave 123 from 123.0000 or 123.3 from 123.300
      if ( valstr.at( lastNotZeroPosition ) == '.' ) {

        --lastNotZeroPosition;
      }
      valstr.erase( lastNotZeroPosition + 1, std::string::npos );
    }
    return valstr;
  }

  std::vector<TimedData> SignalUtils::loadAuxData( const std::string& file ) {
    std::ifstream annofile( file );
    std::string line;
    std::vector<TimedData> data;
    if ( annofile.is_open( ) ) {
      while ( std::getline( annofile, line ) ) {
        size_t spaceidx = line.find( ' ' );
        std::string text( "" );
        dr_time time = 0;
        if ( spaceidx != std::string::npos ) {

          text = line.substr( spaceidx + 1 );
          text = SignalUtils::trim( text );
          time = std::stol( line.substr( 0, spaceidx ) );
        }

        data.push_back( TimedData( time, text ) );
      }
    }
    return data;
  }

  std::vector<std::string> SignalUtils::splitcsv( const std::string& csvline, char delim ) {
    std::vector<std::string> rslt;
    std::stringstream ss( csvline );
    std::string cellvalue;

    while ( std::getline( ss, cellvalue, delim ) ) {

      cellvalue.erase( std::remove( cellvalue.begin( ), cellvalue.end( ), '\r' ), cellvalue.end( ) );
      rslt.push_back( SignalUtils::trim( cellvalue ) );
    }
    return rslt;
  }

  std::vector<std::string_view> SignalUtils::splitcsv( const std::string_view& csvline, char delim ) {
    std::vector<std::string_view> rslt;

    size_t first = 0;
    while ( first < csvline.size( ) ) {
      const auto second = csvline.find( delim, first );
      const std::string_view smallview = ( std::string::npos == second
          ? csvline.substr( first )
          : csvline.substr( first, second - first ) );

      rslt.push_back( smallview );

      if ( std::string::npos == second ) {
        break;
      }

      first = second + 1;
    }

    return rslt;
  }
}
