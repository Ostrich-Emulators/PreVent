/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TdmsReader.h"
#include "DataRow.h"
#include "SignalData.h"

#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>

#include <TdmsParser.h>
#include <TdmsChannel.h>
#include <TdmsGroup.h>
#include <TdmsMetaData.h>

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

void TdmsReader::newGroup( TdmsGroup * grp ) {
}

void TdmsReader::newChannel( TdmsChannel * channel ) {
  //output( ) << "new channel: " << channel->getName( ) << std::endl;
  std::string name = channel->getName( );
  name = name.substr( 2, name.length( ) - 3 );

  //output( ) << "new channel: " << name << std::endl;
  dr_time time = 0;
  const int timeinc = 1024; // philips runs at 1.024s, or 1024 ms
  int freq = 0; // waves have an integer frequency
  auto propmap = channel->getProperties( );

  bool iswave = ( propmap.count( "Frequency" ) > 0 && std::stod( propmap.at( "Frequency" ) ) > 1.0 );

  bool isnew = ( 0 == signalsavers.count( channel ) );
  if ( isnew ) {
    signalsavers.insert( std::make_pair( channel, SignalSaver( name, iswave ) ) );
  }

  // figure out if this is a wave or a vital
  std::unique_ptr<SignalData>& signal = ( iswave
      ? filler->addWave( name )
      : filler->addVital( name ) );

  string unit = channel->getUnit( );
  if ( !unit.empty( ) ) {
    signal->setUom( unit );
  }

  for ( auto& p : propmap ) {
    // output( ) << p.first << " => " << p.second << std::endl;

    if ( "Unit_String" == p.first ) {
      signal->setUom( p.second );
    }
    else if ( "wf_starttime" == p.first ) {
      time = parsetime( p.second );
      if ( time < 0 ) {
        std::cout << name << ": " << p.first << " => " << p.second << std::endl;
      }

      // do not to overwrite our lasttime if we're re-initing after a roll over
      if ( isnew ) {
        signalsavers[channel].lasttime = time;
      }
    }
    else if ( "wf_increment" == p.first ) {
      // ignored--we're forcing 1024ms increments
    }
    else if ( "Frequency" == p.first ) {
      double f = std::stod( p.second );

      freq = ( f < 1 ? 1 : (int) ( f * 1.024 ) );
      signal->setMeta( "Notes", "The frequency from the input file was multiplied by 1.024" );
    }

    signal->setMeta( p.first, p.second );
  }

  //std::cout << signal->name( ) << ( iswave ? " wave" : " vital" ) << "; timeinc: " << timeinc << "; freq: " << freq << std::endl;
  signal->setChunkIntervalAndSampleRate( timeinc, freq );
}

void TdmsReader::newValueChunk( TdmsChannel * channel, std::vector<double>& vals ) {
  SignalSaver& rec = signalsavers.at( channel );
  //output( ) << name << " new values: " << vals.size( ) << std::endl;

  // get our SignalData for this channel
  std::unique_ptr<SignalData>& signal = ( rec.iswave
      ? filler->addWave( rec.name )
      : filler->addVital( rec.name ) );
  int timeinc = signal->metai( ).at( SignalData::CHUNK_INTERVAL_MS );
  size_t freq = signal->metai( ).at( SignalData::READINGS_PER_CHUNK );

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
        // if we have a whole DataRow worth of nans, don't write anything
        rec.nancount++;
      }
      else if ( !rec.seenfloat ) {
        double fraction = std::modf( d, &intpart );
        if ( 0 != fraction ) {
          rec.seenfloat = true;
        }
      }
    }

    writeSignalRow( freq, rec.nancount, doubles, rec.seenfloat, signal, rec.lasttime );
    rec.nancount = 0;
    // check for roll-over
    rec.waiting = ( isRollover( rec.lasttime, rec.lasttime + timeinc ) );
    rec.lasttime += timeinc;
    if ( rec.waiting ) {
      break;
    }
  }

  //output( ) << channel->getName()<<" leaving " << leftover.size( ) << " elements leftover" << std::endl;

  //  else {
  //    // vitals are easy...we don't have to stack values into strings
  //    // and we never have leftovers (yet)
  //    for ( auto& d : vals ) {
  //      if ( !isnan( d ) ) {
  //        // check if our number ends in .000000...
  //        double intpart = 0;
  //        double mantissa = std::modf( d, &intpart );
  //        bool isint = ( 0 == mantissa );
  //        if ( isint ) {
  //          DataRow row( lastTimes[channel], std::to_string( (int) intpart ) );
  //          signal->add( row );
  //        }
  //        else {
  //          DataRow row( lastTimes[channel], std::to_string( d ) );
  //          signal->add( row );
  //        }
  //      }
  //      lastTimes[channel] += timeinc;
  //
  //      // FIXME: check for roll overs
  //    }
}

dr_time TdmsReader::parsetime( const std::string & tmptimestr ) {
  // sample: 14.12.2017 17:49:24,0.000000

  // first: remove the comma and everything after it
  size_t x = tmptimestr.rfind( ',' );
  std::string timestr = tmptimestr.substr( 0, x );

  // there appears to be a bug in the time parser that requires a leading 0
  // for days < 10, so check this situation
  if ( '.' == timestr[1] ) {
    timestr = "0" + timestr;
  }

  tm brokenTime;
  strptime2( timestr, "%d.%m.%Y %H:%M:%S", &brokenTime );
  time_t sinceEpoch = timegm( &brokenTime );
  return sinceEpoch * 1000;
}

int TdmsReader::prepare( const std::string& recordset, std::unique_ptr<SignalSet>& info ) {
  output( ) << "warning: Signals are assumed to be sampled at 1024ms intervals, not 1000ms" << std::endl;

  int rslt = Reader::prepare( recordset, info );
  if ( 0 != rslt ) {
    return rslt;
  }

  parser.reset( new TdmsParser( recordset ) );
  if ( parser->fileOpeningError( ) ) {
    return 1;
  }

  parser->addListener( this );
  return 0;
}

ReadResult TdmsReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastfill ) {
  int retcode = 0;
  filler = info.get( );

  if ( ReadResult::FIRST_READ == lastfill ) {
    parser->init( );
  }
  else if ( ReadResult::END_OF_DAY == lastfill ) {
    // reinit the new SignalSet from our old Channels
    for ( auto&x : signalsavers ) {
      this->newChannel( x.first );
      x.second.waiting = false;
    }
  }

  while ( parser->nextSegment() ) {
    // output()<<"\tjust read a segment"<<std::endl;

    // all the data saving gets done by the listener, not here

    // if we have some signals, are all are "waiting",
    // then we need to roll over the file

    // FIXME: what if the file starts at like 11:58pm, and we only get one
    // signal in the first segment with 3 minutes of data? That is a problem.
    bool allwaiting = ( signalsavers.size( ) > 0 );
    for ( auto&x : signalsavers ) {
      if ( !x.second.waiting ) {
        allwaiting = false;
      }
    }

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

    std::vector<double> none;
    this->newValueChunk( x.first, none );
  }

  parser->close( );

  return ( 0 <= retcode ? ReadResult::END_OF_FILE : ReadResult::ERROR );
}

bool TdmsReader::writeSignalRow( size_t count, size_t nancount, std::vector<double>& doubles,
    const bool seenFloat, const std::unique_ptr<SignalData>& signal, dr_time time ) {

  // make sure we have some data!
  if ( nancount != count ) {
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

    for ( size_t i = 1; i < count; i++ ) {
      vals << ",";
      if ( SignalData::MISSING_VALUE == doubles[i] ) {
        vals << SignalData::MISSING_VALUESTR;
      }
      else {
        vals << doubles[i];
      }
    }

    //output( ) << vals.str( ) << std::endl;
    DataRow row( time, vals.str( ) );
    signal->add( row );
  }

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
