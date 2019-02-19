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
  // output( ) << "reading " << name << std::endl;
  dr_time time = 0;
  const int timeinc = 1024; // philips runs at 1.024s, or 1024 ms
  int freq = 0; // waves have an integer frequency
  auto propmap = channel->getProperties( );

  bool iswave = ( propmap.count( "Frequency" ) > 0 && std::stod( propmap.at( "Frequency" ) ) > 1.0 );

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

  std::cout << signal->name( ) << ( iswave ? " wave" : " vital" ) << "; timeinc: " << timeinc << "; freq: " << freq << std::endl;
  signal->setChunkIntervalAndSampleRate( timeinc, freq );
  std::cout << signal->metai( ).at( SignalData::CHUNK_INTERVAL_MS ) << std::endl;
}

bool TdmsReader::newValue( TdmsChannel * channel, double val ) {
  return false;
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
  for ( auto & chtime : lastTimes ) {
    chtime.first->freeMemory( );
  }
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
    parser->read( false );
  }

  unsigned int groupCount = parser->getGroupCount( );
  for ( unsigned int i = 0; i < groupCount; i++ ) {
    TdmsGroup *group = parser->getGroup( i );
    if ( !group ) {
      continue;
    }

    unsigned int channelsCount = group->getGroupSize( );
    for ( unsigned int j = 0; j < channelsCount; j++ ) {
      // cycle through all the channels, getting one chunk at a time.
      // when we're all out of chunks to read, free the channel
      // and continue on with the others.

      TdmsChannel * ch = group->getChannel( j );
      if ( ch ) {
        std::vector<double> data = ch->nextChunk( );
        if ( !data.empty( ) ) {
          int freq = 1;
          std::string freqstr = ch->getProperty( "Frequency" );
          if ( "" != freqstr ) {
            double f = std::stod( freqstr );
            freq = ( f < 1 ? 1 : (int) ( f * 1.024 ) );
          }

          std::string name = ch->getName( );
          name = name.substr( 2, name.length( ) - 3 );
          // output( ) << "reading " << name << std::endl;
          auto propmap = ch->getProperties( );

          bool iswave = ( propmap.count( "Frequency" ) > 0 && std::stod( propmap.at( "Frequency" ) ) > 1.0 );

          // figure out if this is a wave or a vital
          std::unique_ptr<SignalData>& signal = ( iswave
              ? filler->addWave( name )
              : filler->addVital( name ) );


          int timeinc = signal->metai( ).at( SignalData::CHUNK_INTERVAL_MS );
          unsigned int type = ch->getDataType( );
          dr_time time = lastTimes[ch];

          if ( type == TdmsChannel::tdsTypeComplexSingleFloat
              || type == TdmsChannel::tdsTypeComplexDoubleFloat ) {
            std::cerr << "WARNING: complex data types are not yet supported"
                << std::endl;
            //            std::vector<double> imData = ch->getImaginaryDataVector( );
            //            double iVal1 = imData.front( ), iVal2 = imData.back( );
            //            std::string fmt = ":\n\t%g";
            //            fmt.append( ( iVal1 < 0 ) ? "-i*%g ... %g" : "+i*%g ... %g" );
            //            fmt.append( ( iVal2 < 0 ) ? "-i*%g\n" : "+i*%g\n" );
            //            printf( fmt.c_str( ), data.front( ), fabs( iVal1 ), data.back( ), fabs( iVal2 ) );
          }
          else {
            if ( signal->wave( ) ) {
              // for waves, we need to construct a string of values that is
              // {Frequency} items big

              std::vector<double> doubles;

              // we pretty much always get a datatype of float, even though
              // not all the data IS float
              bool seenFloat = false;
              // if we have a whole datarow worth of nans, don't write anything
              int nancount = 0;
              // how many numbers have we seen?
              int cnt = 0;
              double intpart;
              while ( !data.empty( ) ) {
                for ( auto& d : data ) {
                  bool nan = isnan( d );
                  doubles.push_back( nan ? SignalData::MISSING_VALUE : d );
                  cnt++;

                  if ( nan ) {
                    nancount++;
                  }
                  else {
                    double fraction = std::modf( d, &intpart );
                    if ( 0 != fraction ) {
                      seenFloat = true;
                    }
                  }

                  if ( cnt == freq ) {
                    writeWaveChunkAndReset( cnt, nancount, doubles, seenFloat, signal, time, timeinc );
                  }
                }

                data = ch->nextChunk( );
              }

              if ( 0 != cnt && nancount != cnt ) {
                // uh oh...we have some un-flushed data (probably an interrupted
                // read) so normalize it, then add it
                for (; cnt < freq; cnt++ ) {
                  doubles.push_back( SignalData::MISSING_VALUE );
                }

                writeWaveChunkAndReset( cnt, nancount, doubles, seenFloat, signal, time, timeinc );
              }

            }
            else {
              // vitals are much easier...
              while ( !data.empty( ) ) {
                for ( auto& d : data ) {
                  if ( !isnan( d ) ) {
                    // check if our number ends in .000000...
                    double intpart = 0;
                    double mantissa = std::modf( d, &intpart );
                    bool isint = ( 0 == mantissa );
                    if ( isint ) {
                      DataRow row( time, std::to_string( (int) intpart ) );
                      signal->add( row );
                    }
                    else {
                      DataRow row( time, std::to_string( d ) );
                      signal->add( row );
                    }
                  }
                  time += timeinc;
                }

                data = ch->nextChunk( );
              }
            }
          }
        }
        //        else if ( stringCount ) {
        //          std::cerr << "WARNING: string datasets are not yet supported" << std::endl;
        //          std::vector<std::string> strings = ch->getStringVector( );
        //          string str1 = strings.front( );
        //          if ( str1.empty( ) ){
        //            str1 = "empty string";
        //          }
        //          string str2 = strings.back( );
        //          if ( str2.empty( ) ){
        //            str2 = "empty string";
        //          }
        //          printf( ":\n\t%s ... %s\n", str1.c_str( ), str2.c_str( ) );
        //        }
        //        else{
        //          printf( ".\n" );
        //        }

        ch->freeMemory( );
      }
    }
  }

  // doesn't look like Tdms can do incremental reads, so we're at the end
  return ( 0 <= retcode ? ReadResult::END_OF_FILE : ReadResult::ERROR );
}

bool TdmsReader::writeWaveChunkAndReset( int& count, int& nancount, std::vector<double>& doubles,
    bool& seenFloat, std::unique_ptr<SignalData>& signal, dr_time& time, int timeinc ) {

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

    for ( int i = 1; i < count; i++ ) {
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

  doubles.clear( );
  count = 0;
  nancount = 0;
  seenFloat = false;
  time += timeinc;

  return true;
}
