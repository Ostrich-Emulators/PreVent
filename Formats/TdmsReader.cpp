/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TdmsReader.h"
#include "DataRow.h"
#include "SignalData.h"
#include "Log.h"

#include <math.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
namespace FormatConverter{

  const unsigned long TdmsReader::MAX_WAITING_GAP_MS = 60 * 60 * 1000; // 1 hour in ms

  TdmsReader::TdmsReader( ) : Reader( "TDMS" ), filler( nullptr ) { }

  TdmsReader::~TdmsReader( ) { }

  void TdmsReader::data( const std::string& channelname, const unsigned char* datablock, TDMS::data_type_t datatype, size_t num_vals ) {
    auto vals = std::vector<double>( num_vals );
    if ( nullptr != datablock ) {
      memcpy( vals.data( ), datablock, datatype.length( ) * num_vals );
    }

    //Log::debug( ) << channelname << " new values: " << num_vals << "/" << vals.size( ) << std::endl;
    auto& rec = signalsavers.at( channelname );

    // get our SignalData for this channel
    auto signal = ( rec.iswave
        ? filler->addWave( rec.name )
        : filler->addVital( rec.name ) );
    int timeinc = signal->chunkInterval( );
    size_t freq = signal->readingsPerChunk( );

    // for now, just add whatever we get to our leftovers, and work from there
    rec.leftovers.insert( rec.leftovers.end( ), vals.begin( ), vals.end( ) );
    size_t totalvals = rec.leftovers.size( );

    // we have enough values to make at least one DataRow
    // make as many whole DataRows as we can, and save the leftovers

    // if we're waiting (for other Signals to roll over), everything's a leftover
    if ( totalvals < freq || rec.waiting ) {
      // easiest case: we don't have enough data to
      // write a whole DataRow yet so just move on
      return;
    }

    // we pretty much always get a datatype of float, even though
    // not all the data IS float, by the way
    while ( rec.leftovers.size( ) >= freq ) {
      const auto startpos = rec.leftovers.begin( );
      const auto endpos = startpos + freq;
      auto doubles = std::vector<double>{ startpos, endpos };
      rec.leftovers.erase( startpos, endpos );
      writeSignalRow( doubles, signal, rec.lasttime );

      // check for roll-over
      rec.waiting = ( isRollover( rec.lasttime + timeinc, filler ) );
      rec.lasttime += timeinc;

      if ( rec.waiting ) {
        // output( ) << "\t" << rec.name << " is now waiting (last time: " << rec.lasttime << ")" << std::endl;
        break;
      }
    }
  }

  int TdmsReader::prepare( const std::string& recordset, SignalSet * info ) {
    Log::warn( ) << "warning: Signals are assumed to be sampled at 1024ms intervals, not 1000ms" << std::endl;
    //TDMS::log::debug.debug_mode = true;
    int rslt = Reader::prepare( recordset, info );
    if ( 0 != rslt ) {
      return rslt;
    }

    tdmsfile.reset( new TDMS::tdmsfile( recordset ) );
    last_segment_read = 0;
    return 0;
  }

  void TdmsReader::handleLateStarters( ) {
    // figure out the earliest time

    dr_time earliest = std::numeric_limits<dr_time>::max( );
    for ( auto& ss : signalsavers ) {
      earliest = std::min( ss.second.lasttime, earliest );
    }

    // if we have a signal that starts after a rollover will have occurred,
    // set that signal to waiting
    for ( auto& ss : signalsavers ) {
      ss.second.waiting = ( splitter( ).isRollover( earliest, ss.second.lasttime ) );
    }
  }

  void TdmsReader::initSignal( TDMS::channel * channel, bool firstrun ) {
    //Log::trace( ) << "init new channel: " << channel->get_path( ) << std::endl;
    const auto STARTER = std::string{ "/'Intellivue'/'" };

    // we only accept Intellivue channels
    if ( channel->get_path( ).size( ) < STARTER.size( ) || std::string::npos == channel->get_path( ).find( STARTER ) ) {
      return;
    }

    auto name = channel->get_path( );
    name = name.substr( STARTER.size( ), name.size( ) - STARTER.size( ) - 1 );

    dr_time time = 0;
    const int timeinc = 1024; // philips runs at 1.024s, or 1024 ms
    int freq = 0; // waves have an integer frequency
    auto propmap = channel->get_properties( );

    // if the propmap doesn't contain a Frequency, then we can't use it
    // also: if we've already seen this channel and made a Signal from it,
    // we can move on
    if ( 0 == propmap.count( "Frequency" ) ) {
      return;
    }

    bool iswave = ( propmap.at( "Frequency" )->asDouble( ) > 1.0 );

    if ( firstrun && 0 == signalsavers.count( channel->get_path( ) ) ) {
      // add a saver if we haven't seen this channel before (and it's the first run!)
      signalsavers.insert( std::make_pair( channel->get_path( ), SignalSaver( name, iswave ) ) );
    }

    // figure out if this is a wave or a vital
    bool added = false;
    auto signal = ( iswave
        ? filler->addWave( name, &added )
        : filler->addVital( name, &added ) );

    //  if ( firstrun ) {
    //    output( ) << "  channel: " << name << " props: " << propmap.size( ) << std::endl;
    //  }

    for ( auto& p : propmap ) {
      //    if ( firstrun ) {
      //      output( ) << "\t" << p.first << " => " << p.second << std::endl;
      //    }

      if ( "Unit_String" == p.first ) {
        signal->setUom( p.second->asString( ) );
      }
      else if ( "wf_starttime" == p.first ) {
        // tdsTypeTimeStamps are in seconds, but we need ms for our modifier
        time = modtime( p.second->asUTCTimestamp( ) * 1000 );
        // do not to overwrite our lasttime if we're re-initing after a roll over
        if ( firstrun ) {
          signalsavers.at( channel->get_path( ) ).lasttime = time;
        }
      }
      else if ( "wf_increment" == p.first ) {
        // ignored--we're forcing 1024ms increments
      }
      else if ( "Frequency" == p.first ) {
        double f = p.second->asDouble( );

        freq = ( f < 1 ? 1 : (int) ( f * 1.024 ) );
        signal->setMeta( "Notes", "The frequency from the input file was multiplied by 1.024" );
      }

      TDMS::data_type_t valtype = p.second->data_type;

      if ( valtype.is_string( ) ) {
        signal->setMeta( p.first, p.second->asString( ) );
      }
      else if ( valtype.name( ) == "tdsTypeDoubleFloat" ) {
        signal->setMeta( p.first, p.second->asDouble( ) );
      }
      else if ( valtype.name( ) == "tdsTypeTimeStamp" ) {
        // tdsTypeTimeStamps are in seconds, but we need ms for our modifier, so
        // multiply by 1000. However, time_t expects seconds, so divide by 1000
        time_t timer = modtime( p.second->asUTCTimestamp( ) * 1000 ) / 1000;
        tm * pt = gmtime( &timer );

        char buffer[80];
        snprintf( buffer, 80, "%d.%02d.%d %02d:%02d:%02d,%f",
            pt->tm_mday, pt->tm_mon + 1, 1900 + pt->tm_year, pt->tm_hour, pt->tm_min, pt->tm_sec, 0.0 );
        //std::cout << "  " << p.first << " (timestamp): " << buffer << std::endl;
        signal->setMeta( p.first, std::string( buffer ) );
      }
    }

    //std::cout << signal->name( ) << ( iswave ? " wave" : " vital" ) << "; timeinc: " << timeinc << "; freq: " << freq << std::endl;
    signal->setChunkIntervalAndSampleRate( timeinc, freq );
  }

  ReadResult TdmsReader::fill( SignalSet * info, const ReadResult& lastfill ) {
    filler = info;
    bool firstrun = ( ReadResult::FIRST_READ == lastfill );

    for ( const auto& o : *tdmsfile ) {
      initSignal( o, firstrun );
    }

    handleLateStarters( );

    Log::trace( ) << "at fill, segment: " << last_segment_read << std::endl;
    for ( auto&x : signalsavers ) {
      auto& ss = x.second;
      time_t lastt = ss.lasttime / 1000;
      tm time = *std::gmtime( &lastt );
      Log::trace( ) << std::setfill( ' ' ) << std::setw( 30 ) << ss.name
          << ( ss.waiting ? " WAITING " : " no wait " ) << " "
          << ss.lasttime << " "
          << std::setfill( '0' ) << std::setw( 2 ) << 1900 + time.tm_year << "-"
          << std::setfill( '0' ) << std::setw( 2 ) << time.tm_mon + 1 << "-"
          << std::setfill( '0' ) << std::setw( 2 ) << time.tm_mday << "T"
          << std::setfill( '0' ) << std::setw( 2 ) << time.tm_hour << ":"
          << std::setfill( '0' ) << std::setw( 2 ) << time.tm_min << ":"
          << std::setfill( '0' ) << std::setw( 2 ) << time.tm_sec

          << "Z\tleftovers:" << std::setfill( ' ' ) << std::setw( 7 ) << ss.leftovers.size( )
          << std::endl;
    }

    while ( last_segment_read < tdmsfile->segments( ) ) {
      tdmsfile->loadSegment( last_segment_read, this );
      // all the data saving gets done by the listener, not here

      // if we have some signals, and all are "waiting",
      // then we need to roll over the file

      // FIXME: what if the file starts at like 11:58pm, and we only get one
      // signal in the first segment with 3 minutes of data? That is a problem.

      bool allwaiting = ( signalsavers.size( ) > 0 );
      for ( auto&x : signalsavers ) {
        if ( !x.second.waiting ) {
          allwaiting = false;
        }
      }

      last_segment_read++;
      if ( allwaiting ) {
        return ReadResult::END_OF_DURATION;
      }
    }

    // we're out of segments, so we're done reading the file, but now we need to
    // fill out the last DataRow from the leftovers

    // because we may have a lot (LOT!) of data stacked up waiting for some
    // lagging signal, we need to process these leftover rows as if we were
    // reading them from the file...check for rollover, send back intermediate
    // data, and all that...ugh.
    auto removers = std::vector<std::string>{ };
    for ( auto&x : signalsavers ) {
      auto& rec = x.second;
      auto signal = ( rec.iswave
          ? filler->addWave( rec.name )
          : filler->addVital( rec.name ) );
      size_t freq = signal->metai( ).at( SignalData::READINGS_PER_CHUNK );

      if ( !x.second.leftovers.empty( ) ) {
        if ( x.second.leftovers.size( ) < freq ) {
          // not enough values to fill up a datarow, so add some
          x.second.leftovers.resize( freq, SignalData::MISSING_VALUE );
        }

        this->data( x.first, nullptr, TDMS::data_type_t( "ignored", 0, nullptr ), 0 );
      }

      if ( x.second.leftovers.empty( ) ) {
        removers.push_back( x.first );
      }
    }

    for ( auto& x : removers ) {
      signalsavers.erase( x );
    }

    // now, if all our signalsavers are empty, we're done with this file
    // else we have to loop again
    return ( signalsavers.empty( )
        ? ReadResult::END_OF_FILE
        : ReadResult::END_OF_DURATION );
  }

  bool TdmsReader::writeSignalRow( std::vector<double>& doubles, SignalData * signal, dr_time time ) {

    if ( signal->wave( ) && this->skipwaves( ) ) {
      return true;
    }

    signal->add( std::make_unique<DataRow>( time, doubles ) );

    return true;
  }

  SignalSaver::~SignalSaver( ) { }

  SignalSaver::SignalSaver( const std::string& label, bool wave )
      : waiting( false ), iswave( wave ), name( label ),
      lasttime( std::numeric_limits<dr_time>::min( ) ) { }

  SignalSaver::SignalSaver( const SignalSaver & orig )
      : waiting( orig.waiting ), iswave( orig.iswave ), name( orig.name ),
      lasttime( orig.lasttime ), leftovers( orig.leftovers ) { }

  SignalSaver & SignalSaver::operator=(const SignalSaver & orig ) {
    if ( &orig != this ) {
      waiting = orig.waiting;
      iswave = orig.iswave;
      name = orig.name;
      lasttime = orig.lasttime;
      leftovers = orig.leftovers;
    }

    return *this;
  }
}