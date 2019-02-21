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

TdmsReader::TdmsReader( ) : Reader( "TDMS" ), filler( nullptr ), lastSaveTime( 0 ) {
}

TdmsReader::~TdmsReader( ) {

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

  if ( iswave ) {
    wavesave.insert( std::make_pair( channel, WaveRecord( ) ) );
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

      lastTimes.insert( std::make_pair( channel, time ) );
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
  std::string name = channel->getName( );
  name = name.substr( 2, name.length( ) - 3 );
  //output( ) << name << " new values: " << vals.size( ) << std::endl;

  auto propmap = channel->getProperties( );
  bool iswave = ( propmap.count( "Frequency" ) > 0 && std::stod( propmap.at( "Frequency" ) ) > 1.0 );

  // get our SignalData for this channel
  std::unique_ptr<SignalData>& signal = ( iswave
      ? filler->addWave( name )
      : filler->addVital( name ) );
  int timeinc = signal->metai( ).at( SignalData::CHUNK_INTERVAL_MS );
  size_t freq = signal->metai( ).at( SignalData::READINGS_PER_CHUNK );

  if ( iswave ) {
    // for waves, we need to construct a string of values that is
    // {Frequency} items big

    // for now, just add whatever we get to our leftovers, and work from there
    auto& leftover = leftovers[channel];
    leftover.insert( leftover.end( ), vals.begin( ), vals.end( ) );
    size_t totalvals = leftover.size( );

    if ( totalvals < freq ) {
      // easiest case: we don't have enough data to
      // write a whole DataRow yet so just move on
      return;
    }

    // we have enough values to make at least one DataRow
    // make as many whole DataRows as we can, and save the leftovers

    // we pretty much always get a datatype of float, even though
    // not all the data IS float, by the way

    double intpart;
    while ( leftover.size( ) >= freq ) {
      std::vector<double> doubles;
      doubles.reserve( freq );
      for ( size_t i = 0; i < freq; i++ ) {
        double d = leftover.front( );
        leftover.pop_front( );
        bool nan = isnan( d );
        doubles.push_back( nan ? SignalData::MISSING_VALUE : d );

        if ( nan ) {
          // if we have a whole DataRow worth of nans, don't write anything
          wavesave[channel].nancount++;
        }
        else if ( !wavesave[channel].seenfloat ) {
          double fraction = std::modf( d, &intpart );
          if ( 0 != fraction ) {
            wavesave[channel].seenfloat = true;
          }
        }
      }

      writeWaveChunk( freq, wavesave[channel].nancount, doubles,
          wavesave[channel].seenfloat, signal, lastTimes[channel] );
      lastTimes[channel] += timeinc;
      wavesave[channel].nancount = 0;
    }

    //output( ) << channel->getName()<<" leaving " << leftover.size( ) << " elements leftover" << std::endl;
  }
  else {
    // vitals are easy...we don't have to stack values into strings
    // and we never have leftovers (yet)
    for ( auto& d : vals ) {
      if ( !isnan( d ) ) {
        // check if our number ends in .000000...
        double intpart = 0;
        double mantissa = std::modf( d, &intpart );
        bool isint = ( 0 == mantissa );
        if ( isint ) {
          DataRow row( lastTimes[channel], std::to_string( (int) intpart ) );
          signal->add( row );
        }
        else {
          DataRow row( lastTimes[channel], std::to_string( d ) );
          signal->add( row );
        }
      }
      lastTimes[channel] += timeinc;
    }
  }


  //  int count = 0;
  //  while ( count < vals.size( ) ) {
  //    if ( 0 == count % 25 ) {
  //      output( ) << std::endl << "\t";
  //    }
  //
  //    output( ) << vals[count++] << " ";
  //  }
  //  output( ) << std::endl;
}

void TdmsReader::startSaving( dr_time savetime ) {
  if ( savetime != lastSaveTime ) {
    saved.setMetadataFrom( *filler );
    filler = &saved;
    filler->clearOffsets( );
    lastSaveTime = savetime;
  }
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
  output( ) << "warning: TDMS reader cannot split dataset by day (...yet)" << std::endl;
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

void TdmsReader::finish( ) {
  parser->close( );
}

void TdmsReader::copySavedInto( std::unique_ptr<SignalSet>& tgt ) {
  // copy all our saved data into this new tgt signalset
  tgt->setMetadataFrom( saved );
  saved.clearMetas( );

  for ( auto& m : saved.vitals( ) ) {
    const std::unique_ptr<SignalData>& savedsignal = m;

    bool added = false;
    std::unique_ptr<SignalData>& infodata = tgt->addVital( m->name( ), &added );

    infodata->setMetadataFrom( *savedsignal );
    int rows = savedsignal->size( );
    for ( int row = 0; row < rows; row++ ) {
      const std::unique_ptr<DataRow>& datarow = savedsignal->pop( );
      infodata->add( *datarow );
    }
  }

  for ( auto& m : saved.waves( ) ) {
    const std::unique_ptr<SignalData>& savedsignal = m;

    bool added = false;
    std::unique_ptr<SignalData>& infodata = tgt->addWave( m->name( ), &added );

    infodata->setMetadataFrom( *savedsignal );
    int rows = savedsignal->size( );
    for ( int row = 0; row < rows; row++ ) {
      const std::unique_ptr<DataRow>& datarow = savedsignal->pop( );
      infodata->add( *datarow );
    }
  }

  saved.reset( false );
}

ReadResult TdmsReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastfill ) {
  int retcode = 0;

  if ( ReadResult::END_OF_DAY == lastfill || ReadResult::END_OF_PATIENT == lastfill ) {
    copySavedInto( info );
  }
  filler = info.get( );

  if ( ReadResult::FIRST_READ == lastfill ) {
    parser->init();
  }

  while( parser->nextSegment() ){
    //output()<<"\tjust read a segment"<<std::endl;

    // all the data saving gets done by the listener, not here

    // FIXME: check for roll-over
  }

  

  // we're done reading the file, but now we need to fill out the last DataRow
  // from our leftovers
  for ( auto&x : leftovers ) {
    std::string name = x.first->getName( );
    name = name.substr( 2, name.length( ) - 3 );
    auto propmap = x.first->getProperties( );
    bool iswave = ( propmap.count( "Frequency" ) > 0 && std::stod( propmap.at( "Frequency" ) ) > 1.0 );

    // get our SignalData for this channel
    std::unique_ptr<SignalData>& signal = ( iswave
        ? filler->addWave( name )
        : filler->addVital( name ) );
    size_t freq = signal->metai( ).at( SignalData::READINGS_PER_CHUNK );
    size_t cnt = x.second.size();
    for( size_t i=cnt; i< freq; i++ ){
      x.second.push_back( SignalData::MISSING_VALUE );
    }

    std::vector<double> none;
    this->newValueChunk( x.first, none );
  }

  // doesn't look like Tdms can do incremental reads, so we're at the end
  return ( 0 <= retcode ? ReadResult::END_OF_FILE : ReadResult::ERROR );
}

bool TdmsReader::writeWaveChunk( size_t count, size_t nancount, std::vector<double>& doubles,
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

WaveRecord::~WaveRecord( ) {
}

WaveRecord::WaveRecord( ) : seenfloat( false ), nancount( 0 ) {
}

WaveRecord::WaveRecord( const WaveRecord& orig )
: seenfloat( orig.seenfloat ), nancount( orig.nancount ) {
}

WaveRecord& WaveRecord::operator=(const WaveRecord& orig ) {
  if ( &orig != this ) {
    seenfloat = orig.seenfloat;
    nancount = orig.nancount;
  }

  return *this;
}
