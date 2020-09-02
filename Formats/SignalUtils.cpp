/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalUtils.h"
#include "TimeRange.h"

#include "SignalData.h"
#include "DataRow.h"
#include "Reader.h"
#include "Log.h"

#include <set>
#include <limits>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>

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

  dr_time SignalUtils::firstlast( const std::map<std::string, SignalData *> map,
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

  dr_time SignalUtils::firstlast( const std::vector<SignalData *> signals,
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

  std::vector<std::vector<int>> SignalUtils::syncDatas( std::vector<SignalData *> data ) {

    auto ret = std::vector<std::vector<int>>( data.size( ) );

    dr_time earliest;
    dr_time latest;
    firstlast( data, &earliest, &latest );

    std::unique_ptr<DataRow> currenttimes[data.size( )];
    for ( size_t i = 0; i < data.size( ); i++ ) {
      // load the first row for each signal into our current array
      currenttimes[i] = data[i]->pop( );
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
            const auto& ints = currenttimes[i]->ints( );
            ret[i].insert( ret[i].end( ), ints.begin( ), ints.end( ) );

            if ( data[i]->empty( ) ) {
              empties.insert( i );
            }
            else {
              currenttimes[i] = data[i]->pop( );
            }
          }
          else if ( currenttimes[i]->time > earliest ) {
            // don't have a datapoint for this time, so make a dummy one
            const auto& ints = dummyfill( data[i], earliest )->ints( );
            ret[i].insert( ret[i].end( ), ints.begin( ), ints.end( ) );
          }
        }
        else {
          // ran out of times fo this signal, so make dummy data for this time
          const auto& ints = dummyfill( data[i], earliest )->ints( );
          ret[i].insert( ret[i].end( ), ints.begin( ), ints.end( ) );
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

    return ret;
  }

  std::map<std::string, SignalData *> SignalUtils::mapify( std::vector<SignalData *> data ) {
    std::map<std::string, SignalData *> map;
    for ( auto& s : data ) {
      map[s->name( )] = s;
    }

    return map;

  }

  std::vector<SignalData *> SignalUtils::vectorize( std::map<std::string, SignalData *> data ) {
    auto vec = std::vector<SignalData *>( );
    for ( auto& m : data ) {
      vec.push_back( m.second );
    }

    return vec;
  }

  std::vector<std::unique_ptr<SignalData>> SignalUtils::sync( std::vector<SignalData *> data ) {

    auto ret = std::vector<std::unique_ptr < SignalData >> ( );

    dr_time earliest;
    dr_time latest;
    firstlast( data, &earliest, &latest );

    //  std::cout << "f/l:\t" << earliest << "\t" << latest << std::endl;
    //  for ( const auto& s : data ) {
    //    std::cout << s->name( ) << "\t" << s->startTime( ) << "\t"
    //        << s->endTime( ) << "\t" << s->size( ) << std::endl;
    //  }

    std::unique_ptr<DataRow> currenttimes[data.size( )];
    for ( size_t i = 0; i < data.size( ); i++ ) {
      ret.push_back( data[i]->shallowcopy( ) );

      // load the first row for each signal into our current array
      currenttimes[i] = data[i]->pop( );
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
              currenttimes[i] = data[i]->pop( );
            }
          }
          else if ( currenttimes[i]->time > earliest ) {
            // don't have a datapoint for this time, so make a dummy one
            ret[i]->add( dummyfill( data[i], earliest ) );
          }
        }
        else {
          // ran out of times fo this signal, so make dummy data for this time
          ret[i]->add( dummyfill( data[i], earliest ) );
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

    return ret;
  }

  std::unique_ptr<DataRow> SignalUtils::dummyfill( SignalData * signal, const dr_time& time ) {
    return std::make_unique<DataRow>( time, std::vector<int>( signal->hz( ), SignalData::MISSING_VALUE ) );
  }

  std::vector<size_t> SignalUtils::index( const std::vector<dr_time>& alltimes,
      SignalData& signal ) {

    auto range = signal.times( );
    auto signaltimes = std::deque<dr_time>{ };
    for ( auto t : *( range.get( ) ) ) {
      signaltimes.push_back( t );
    }
    std::sort( signaltimes.begin( ), signaltimes.end( ) );
    const double hz = signal.hz( );
    const int rowsPerTime = ( hz < 1.0 ? 1 : (int) hz );

    std::vector<size_t> indexes;

    size_t currentIndex = 0;
    for ( dr_time all : alltimes ) {
      Log::trace( ) << "all: " << all << "\t front: " << signaltimes.front( ) << std::endl;
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

  std::unique_ptr<CachefileData> SignalUtils::tmpf( ) {
    auto tmpdir = Options::get( OptionsKey::TMPDIR );
    auto tmppath = std::filesystem::path{ tmpdir };
    tmppath /= "fmtcnv-XXXXXX";
    auto filename = tmppath.string( );
    auto fd = mkstemp( filename.data( ) );
    Log::trace( ) << "creating temp file: " << filename << std::endl;
    return std::make_unique<CachefileData>( filename, fdopen( fd, "wb+" ) );
  }

  std::filesystem::path SignalUtils::canonicalizePath( const std::string& userpath ) {
    auto canonical = std::filesystem::path{ userpath };

#ifdef __CYGWIN__
    size_t size = cygwin_conv_path( CCP_WIN_A_TO_POSIX | CCP_RELATIVE, userpath.data( ), NULL, 0 );
    if ( size < 0 ) {
      throw std::runtime_error( "cannot resolve path: " + userpath );
    }

    char * cygpath = (char *) malloc( size );
    if ( cygwin_conv_path( CCP_WIN_A_TO_POSIX | CCP_RELATIVE, userpath.data( ),
        cygpath, size ) ) {
      free( cygpath );
      throw std::runtime_error( "error converting path: " + userpath );
    }
    //std::cout << "cygpath: " << cygpath << std::endl;
    canonical = cygpath;
    free( cygpath );
#endif

    return canonical;
  }

  CachefileData::CachefileData( const std::string& name, FILE * f ) : filename( name ), file( f ) { }

  CachefileData::~CachefileData( ) {
    if ( nullptr != file ) {
      Log::trace( ) << "closing and removing temp file: " << filename << std::endl;
      std::fclose( file );
      std::remove( filename.data( ) );
    }
  }
}
