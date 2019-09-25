/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TdmsReader.h"
#include "DataRow.h"
#include "SignalData.h"

#include <math.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
namespace FormatConverter {

  TdmsReader::TdmsReader( ) : Reader( "TDMS" ), filler( nullptr ) {
  }

  TdmsReader::~TdmsReader( ) {

  }

  bool TdmsReader::isRollover( const dr_time& then, const dr_time& now ) const {
    if ( nonbreaking( ) ) {
      return false;
    }

    if ( 0 != then ) {
      time_t modnow = now / 1000;
      time_t modthen = then / 1000;

      const int cdoy = ( localizingTime( ) ? localtime( &modnow ) : gmtime( &modnow ) )->tm_yday;
      const int pdoy = ( localizingTime( ) ? localtime( &modthen ) : gmtime( &modthen ) )->tm_yday;
      if ( cdoy != pdoy ) {
        return true;
      }
    }

    return false;
  }

  void TdmsReader::data( const std::string& channelname, const unsigned char* datablock, TDMS::data_type_t datatype, size_t num_vals ) {
    std::vector<double> vals;
    if ( nullptr != datablock ) {
      vals.reserve( num_vals );

      for ( size_t i = 0; i < num_vals; i++ ) {
        double out;
        memcpy( &out, datablock + ( i * datatype.length ), datatype.length );
        vals.push_back( out );
      }
      //    memcpy( &vals[0], datablock, datatype.length * num_vals );
    }

    //output( ) << channelname << " new values: " << num_vals << "/" << vals.size( ) << std::endl;
    SignalSaver& rec = signalsavers.at( channelname );

    // get our SignalData for this channel
    std::unique_ptr<SignalData>& signal = ( rec.iswave
            ? filler->addWave( rec.name )
            : filler->addVital( rec.name ) );
    int timeinc = signal->chunkInterval( );
    size_t freq = signal->readingsPerChunk( );

    //  if ( !signal->wave( ) ) {
    //    output( ) << signal->name( ) << " " << vals.size( )
    //        << " new values to add to " << rec.leftovers.size( )
    //        << " leftovers; lasttime: " << rec.lasttime
    //        << " " << timeinc << std::endl;
    //  }

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

    // for waves, we need to construct a string of values that is
    // {Frequency} items big. For vitals, we do the exact same thing,
    // except that {Frequency} is 1.

    // we pretty much always get a datatype of float, even though
    // not all the data IS float, by the way

    // if we rolled over to a new day, stop entering data to yesterday
    double intpart;
    while ( rec.leftovers.size( ) >= freq ) {
      std::vector<double> doubles;
      doubles.reserve( freq );
      for ( size_t i = 0; i < freq; i++ ) {
        double d = rec.leftovers.front( );
        rec.leftovers.pop_front( );
        bool nan = isnan( d );
        doubles.push_back( nan ? SignalData::MISSING_VALUE : d );

        if ( nan ) {
          // keep nan count because if we have a whole DataRow
          // worth of nans we don't want write anything
          rec.nancount++;
        }
        else if ( !rec.seenfloat ) {
          double fraction = std::modf( d, &intpart );
          if ( 0 != fraction ) {
            rec.seenfloat = true;
          }
        }
      }

      if ( doubles.size( ) != rec.nancount ) {
        writeSignalRow( doubles, rec.seenfloat, signal, rec.lasttime );
      }
      rec.nancount = 0;

      // check for roll-over
      rec.waiting = ( isRollover( rec.lasttime, rec.lasttime + timeinc ) );
      rec.lasttime += timeinc;

      if ( rec.waiting ) {
        // output( ) << "\t" << rec.name << " is now waiting (last time: " << rec.lasttime << ")" << std::endl;
        break;
      }
    }
  }

  int TdmsReader::prepare( const std::string& recordset, std::unique_ptr<SignalSet>& info ) {
    output( ) << "warning: Signals are assumed to be sampled at 1024ms intervals, not 1000ms" << std::endl;
    //TDMS::log::debug.debug_mode = true;
    int rslt = Reader::prepare( recordset, info );
    if ( 0 != rslt ) {
      return rslt;
    }

    tdmsfile.reset( new TDMS::file( recordset ) );
    last_segment_read = 0;
    return 0;
  }

  void TdmsReader::handleLateStarters( ) {
    // figure out the earliest time

    dr_time earliest = std::numeric_limits<dr_time>::max( );
    for ( auto& ss : signalsavers ) {
      if ( ss.second.lasttime < earliest ) {
        earliest = ss.second.lasttime;
      }
    }

    // if we have a signal that starts after a rollover will have occurred,
    // set that signal to waiting
    for ( auto& ss : signalsavers ) {
      ss.second.waiting = ( isRollover( ss.second.lasttime, earliest ) );
    }
  }

  void TdmsReader::initSignal( TDMS::object * channel, bool firstrun ) {
    const std::string starter( "/Intellivue'/'" );
    if ( channel->get_path( ).size( ) < starter.size( ) ) {
      return;
    }

    //output( ) << "new channel: " << channel->get_path( ) << std::endl;
    //output( ) << "new channel: " << channel->getName( ) << std::endl;
    std::string name = channel->get_path( );
    name = name.substr( starter.size( ) + 1, name.size( ) - starter.size( ) - 2 );

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
    std::unique_ptr<SignalData>& signal = ( iswave
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
        time = modtime( p.second->asUTCTimestamp( )* 1000 );
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

      if ( valtype.name == "tdsTypeString" ) {
        signal->setMeta( p.first, p.second->asString( ) );
      }
      else if ( valtype.name == "tdsTypeDoubleFloat" ) {
        signal->setMeta( p.first, p.second->asDouble( ) );
      }
      else if ( valtype.name == "tdsTypeTimeStamp" ) {
        time_t timer = modtime( p.second->asUTCTimestamp( ) );
        tm * pt = gmtime( &timer );

        char buffer[80];
        sprintf( buffer, "%d.%02d.%d %02d:%02d:%02d,%f",
                pt->tm_mday, pt->tm_mon + 1, 1900 + pt->tm_year, pt->tm_hour, pt->tm_min, pt->tm_sec, 0.0 );
        //std::cout << "  " << p.first << " (timestamp): " << buffer << std::endl;
        signal->setMeta( p.first, std::string( buffer ) );
      }
    }

    //std::cout << signal->name( ) << ( iswave ? " wave" : " vital" ) << "; timeinc: " << timeinc << "; freq: " << freq << std::endl;
    signal->setChunkIntervalAndSampleRate( timeinc, freq );
  }

  ReadResult TdmsReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastfill ) {
    int retcode = 0;
    filler = info.get( );
    bool firstrun = ( ReadResult::FIRST_READ == lastfill );

    for ( TDMS::object* o : *tdmsfile ) {
      initSignal( o, firstrun );
    }

    handleLateStarters( );

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
        return ReadResult::END_OF_DAY;
      }
    }

    // we're out of segments, so we're done reading the file, but now we need to
    // fill out the last DataRow from the leftovers
    for ( auto&x : signalsavers ) {
      SignalSaver& rec = x.second;
      // get our SignalData for this channel
      std::unique_ptr<SignalData>& signal = ( rec.iswave
              ? filler->addWave( rec.name )
              : filler->addVital( rec.name ) );
      size_t freq = signal->metai( ).at( SignalData::READINGS_PER_CHUNK );
      size_t cnt = rec.leftovers.size( );
      for ( size_t i = cnt; i < freq; i++ ) {
        rec.leftovers.push_back( SignalData::MISSING_VALUE );
      }

      this->data( x.first, nullptr, TDMS::data_type_t( "ignored", 0, nullptr ), 0 );
    }

    return ( 0 <= retcode ? ReadResult::END_OF_FILE : ReadResult::ERROR );
  }

  bool TdmsReader::writeSignalRow( std::vector<double>& doubles, const bool seenFloat,
          const std::unique_ptr<SignalData>& signal, dr_time time ) {

    std::stringstream vals;
    if ( seenFloat ) {
      // tdms file seems to use 3 decimal places for everything
      // so make sure we don't have extra 0s running around
      vals << std::setprecision( 3 ) << std::fixed;
    }

    if ( SignalData::MISSING_VALUE == doubles[0] ) {
      vals << SignalData::MISSING_VALUESTR;
    }
    else {
      vals << doubles[0];
    }

    for ( size_t i = 1; i < doubles.size( ); i++ ) {
      vals << ",";
      if ( SignalData::MISSING_VALUE == doubles[i] ) {
        vals << SignalData::MISSING_VALUESTR;
      }
      else {
        vals << doubles[i];
      }
    }

    //output( ) << vals.str( ) << std::endl;
    FormatConverter::DataRow row( time, vals.str( ) );
    signal->add( row );

    return true;
  }

  SignalSaver::~SignalSaver( ) {
  }

  SignalSaver::SignalSaver( const std::string& label, bool wave )
  : seenfloat( false ), nancount( 0 ), waiting( false ), iswave( wave ),
  name( label ), lasttime( 0 ) {
  }

  SignalSaver::SignalSaver( const SignalSaver& orig )
  : seenfloat( orig.seenfloat ), nancount( orig.nancount ), waiting( orig.waiting ),
  iswave( orig.iswave ), name( orig.name ), lasttime( orig.lasttime ),
  leftovers( orig.leftovers ) {
  }

  SignalSaver& SignalSaver::operator=(const SignalSaver& orig ) {
    if ( &orig != this ) {
      seenfloat = orig.seenfloat;
      nancount = orig.nancount;
      waiting = orig.waiting;
      iswave = orig.iswave;
      name = orig.name;
      lasttime = orig.lasttime;
      leftovers = orig.leftovers;
    }

    return *this;
  }
}