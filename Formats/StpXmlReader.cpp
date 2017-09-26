/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CacheFileHdf5Writer.cpp
 * Author: ryan
 * 
 * Created on August 26, 2016, 12:55 PM
 * 
 * Almost all the zlib code was taken from http://www.zlib.net/zlib_how.html
 */

#include "StpXmlReader.h"
#include "SignalData.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <map>
#include <memory>

const std::string StpXmlReader::MISSING_VALUESTR( "-32768" );
const std::set<std::string> StpXmlReader::Hz60({ "RR", "VNT_PRES", "VNT_FLOW" } );
const std::set<std::string> StpXmlReader::Hz120({ "ICP1", "ICP2", "ICP4", "LA4" } );
std::string StpXmlReader::working;
const int StpXmlReader::INDETERMINATE = 0;
const int StpXmlReader::INHEADER = 1;
const int StpXmlReader::INVITAL = 2;
const int StpXmlReader::INWAVE = 4;
const int StpXmlReader::INNAME = 8;

StpXmlReader::StpXmlReader( ) : warnMissingName( true ), warnJunkData( true ),
prevtime( 0 ), currvstime( 0 ), lastvstime( 0 ), currwavetime( 0 ), lastwavetime( 0 ),
state( INDETERMINATE ) {
}

StpXmlReader::StpXmlReader( const StpXmlReader& orig ) {
}

StpXmlReader::~StpXmlReader( ) {
}

void StpXmlReader::finish( ) {
  XML_ParserFree( parser );
}

size_t StpXmlReader::getSize( const std::string& input ) const {
  struct stat info;

  if ( stat( input.c_str( ), &info ) < 0 ) {
    perror( input.c_str( ) );
    return 0;
  }

  return info.st_size;
}

void StpXmlReader::start( void * data, const char * el, const char ** attr ) {
  std::map<std::string, std::string> attrs;
  for ( int i = 0; attr[i]; i += 2 ) {
    attrs[attr[i]] = attr[i + 1];
  }

  StpXmlReader * rdr = static_cast<StpXmlReader *> ( data );
  rdr->start( el, attrs );
}

void StpXmlReader::end( void * data, const char * el ) {
  StpXmlReader * rdr = static_cast<StpXmlReader *> ( data );
  rdr->end( el, trim( working ) );
  working.clear( );
}

void StpXmlReader::chars( void * data, const char * text, int len ) {
  working.append( text, len );
}

std::string StpXmlReader::trim( std::string & totrim ) {
  // ltrim
  totrim.erase( totrim.begin( ), std::find_if( totrim.begin( ), totrim.end( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );

  // rtrim
  totrim.erase( std::find_if( totrim.rbegin( ), totrim.rend( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), totrim.end( ) );

  return totrim;
}

void StpXmlReader::setstate( int st ) {
  state = st;
}

void StpXmlReader::start( const std::string& element, std::map<std::string, std::string>& attrs ) {
  if ( "FileInfo" == element ) {
    setstate( INHEADER );
  }
  else if ( "VitalSigns" == element ) {
    setstate( INVITAL );
    lastvstime = currvstime;
    currvstime = time( attrs["Time"] );
    if ( isRollover( lastvstime, currvstime ) ) {
      rslt = ReadResult::END_OF_DAY;
      prevtime = currvstime;
      saved.setMetadataFrom( *filler );
      filler = &saved;
    }
  }
  else if ( "Waveforms" == element ) {
    setstate( INWAVE );
    lastwavetime = currwavetime;
    currwavetime = time( attrs["Time"] );
    if ( isRollover( lastwavetime, currwavetime ) ) {
      rslt = ReadResult::END_OF_DAY;
      prevtime = currwavetime;
      saved.setMetadataFrom( *filler );
      filler = &saved;
    }
  }
  else if ( "PatientName" == element ) {
    setstate( INNAME );
  }
  else if ( "Value" == element ) {
    if ( 0 != attrs.count( "UOM" ) ) {
      uom = attrs["UOM"];
    }
    if ( 0 != attrs.count( "Q" ) ) {
      q = attrs["Q"];
    }
  }
  else if ( "WaveformData" == element ) {
    label = attrs["Channel"];
    uom = attrs["UOM"];
  }

  //  std::cout << "start " << element << std::endl;
  //  if ( !attrs.empty( ) ) {
  //    for ( auto& m : attrs ) {
  //      std::cout << "\t" << m.first << "=>" << m.second << std::endl;
  //    }
  //  }
}

void StpXmlReader::end( const std::string& element, const std::string& text ) {
  if ( INHEADER == state ) {
    if ( "FileInfo" == element ) {
      setstate( INDETERMINATE );
    }
    else if ( "Size" != element ) {
      filler->metadata( )[element] = text;
    }
  }
  else if ( INNAME == state ) {
    if ( 0 == prevtime ) {
      filler->metadata( )["Patient Name"] = text;
    }
    else {
      std::string pname = filler->metadata( )["Patient Name"];
      if ( text != pname ) {
        rslt = ReadResult::END_OF_PATIENT;
        // we've cut over to a new set of data, so
        // save the data we parse now to a different SignalSet
        saved.setMetadataFrom( *filler );
        saved.metadata( )["Patient Name"] = text;
        filler = &saved;
      }
    }
    setstate( INDETERMINATE );
  }
  else if ( INVITAL == state ) {
    if ( "Par" == element ) {
      label = text;
    }
    else if ( "Value" == element ) {
      value = text;
    }
    else if ( "VS" == element ) {
      bool added = false;
      std::unique_ptr<SignalData>& sig = filler->addVital( label, &added );

      if ( added ) {
        sig->metad( ).insert( std::make_pair( SignalData::HERTZ, 0.5 ) );
        sig->metas( ).insert( std::make_pair( SignalData::MSM, MISSING_VALUESTR ) );
        sig->metas( ).insert( std::make_pair( SignalData::TIMEZONE, "UTC" ) );

        if ( !uom.empty( ) ) {
          sig->setUom( uom );
        }
      }

      sig->add( DataRow( currvstime, value ) );
    }
    else if ( "VitalSigns" == element ) {
      setstate( INDETERMINATE );
    }
  }
  else if ( INWAVE == state ) {
    if ( "WaveformData" == element ) {
      if ( label.empty( ) ) {
        if ( warnMissingName ) {
          std::cerr << "ignoring unnamed waveforms" << std::endl;
          warnMissingName = false;
        }
      }
      else if ( shouldExtract( label ) ) {
        std::string wavepoints = text;
        // reverse the oversampling (if any) and set the true Hz
        int thz = 240;
        if ( 0 != Hz60.count( label ) ) {
          wavepoints = resample( text, 60 );
          thz = 60;
        }
        else if ( 0 != Hz120.count( label ) ) {
          wavepoints = resample( text, 120 );
          thz = 120;
        }
        const int hz = thz;

        if ( waveIsOk( wavepoints ) ) {
          bool first;
          std::unique_ptr<SignalData>& sig = filler->addWave( label, &first );
          if ( first ) {
            sig->metas( ).insert( std::make_pair( SignalData::MSM, MISSING_VALUESTR ) );
            sig->metas( ).insert( std::make_pair( SignalData::TIMEZONE, "UTC" ) );

            if ( sig->uom( ).empty( ) && !uom.empty( ) ) {
              sig->setUom( uom );
            }
          }

          sig->metad( )[SignalData::HERTZ] = hz;
          sig->add( DataRow( currwavetime, wavepoints ) );
        }
        else if ( warnJunkData ) {

          warnJunkData = false;
          std::cerr << "skipping waveforms with no usable data" << std::endl;
        }
      }
    }
    else if ( "WaveformData" == element ) {
      setstate( INDETERMINATE );
    }
  }

  // std::cout << "end " << element << std::endl; //"(" << text << ")" << std::endl;
}

int StpXmlReader::prepare( const std::string& fname, SignalSet& info ) {
  int rr = Reader::prepare( fname, info );
  if ( 0 == rr ) {

    parser = XML_ParserCreate( NULL );
    XML_SetUserData( parser, this );
    XML_SetElementHandler( parser, StpXmlReader::start, StpXmlReader::end );
    XML_SetCharacterDataHandler( parser, StpXmlReader::chars );
    input.open( fname, std::ifstream::in );
    rslt = ReadResult::NORMAL;
  }
  return rr;
}

void StpXmlReader::copysaved( SignalSet& tgt ) {
  tgt.setMetadataFrom( saved );
  saved.metadata( ).clear( );

  for ( auto& m : saved.vitals( ) ) {
    std::unique_ptr<SignalData>& savedsignal = m.second;

    bool added = false;
    std::unique_ptr<SignalData>& infodata = tgt.addVital( m.first, &added );

    if ( added ) {
      auto& smap = savedsignal->metas( );
      infodata->metas( ).insert( smap.begin( ), smap.end( ) );

      auto& dmap = savedsignal->metad( );
      infodata->metad( ).insert( dmap.begin( ), dmap.end( ) );

      auto& imap = savedsignal->metai( );
      infodata->metai( ).insert( imap.begin( ), imap.end( ) );
    }

    int rows = savedsignal->size( );
    for ( int row = 0; row < rows; row++ ) {
      const std::unique_ptr<DataRow>& datarow = savedsignal->pop( );
      infodata->add( *datarow );
    }
  }

  for ( auto& m : saved.waves( ) ) {
    std::unique_ptr<SignalData>& savedsignal = m.second;

    bool added = false;
    std::unique_ptr<SignalData>& infodata = tgt.addWave( m.first, &added );

    if ( added ) {
      auto& smap = savedsignal->metas( );
      infodata->metas( ).insert( smap.begin( ), smap.end( ) );

      auto& dmap = savedsignal->metad( );
      infodata->metad( ).insert( dmap.begin( ), dmap.end( ) );

      auto& imap = savedsignal->metai( );
      infodata->metai( ).insert( imap.begin( ), imap.end( ) );
    }

    int rows = savedsignal->size( );
    for ( int row = 0; row < rows; row++ ) {

      const std::unique_ptr<DataRow>& datarow = savedsignal->pop( );
      infodata->add( *datarow );
    }
  }

  saved.vitals( ).clear( );
  saved.waves( ).clear( );
}

ReadResult StpXmlReader::fill( SignalSet & info, const ReadResult& lastfill ) {
  if ( ReadResult::END_OF_DAY == lastfill || ReadResult::END_OF_PATIENT == lastfill ) {
    copysaved( info );
  }
  filler = &info;
  rslt = ReadResult::NORMAL;

  const int buffsz = 16384;
  std::vector<char> buffer( buffsz, 0 );
  while ( input.read( buffer.data( ), buffer.size( ) ) ) {
    long gcnt = input.gcount( );
    XML_Parse( parser, &buffer[0], buffer.size( ), gcnt < buffsz );
    if ( ReadResult::NORMAL != rslt ) {
      return rslt;
    }
  }

  return ReadResult::END_OF_FILE;
}

bool StpXmlReader::isRollover( const time_t& then, const time_t& now ) const {
  if ( 0 != then ) {
    const int cdoy = gmtime( &now )->tm_yday;
    const int pdoy = gmtime( &then )->tm_yday;
    if ( cdoy != pdoy ) {

      return true;
    }
  }

  return false;
}

time_t StpXmlReader::time( const std::string& timer ) const {
  if ( std::string::npos == timer.find( " " ) ) {
    return std::stol( timer );
  }

  // we have a local time that we need to convert
  tm mytime;
  strptime( timer.c_str( ), "%m/%d/%Y %I:%M:%S %p", &mytime );
  // now convert our local time to UTC
  time_t local = mktime( &mytime );
  mytime = *gmtime( &local );

  return mktime( &mytime );
}

std::string StpXmlReader::resample( const std::string& data, int hz ) {
  /**
  The data arg is always sampled at 240 Hz and is in a 2-second block (480 vals)
  However, the actual wave sampling rate may be less (will always be a multiple of 60Hz)
  so we need to remove the extra values if the values are up-sampled
   **/

  if ( !( 60 == hz || 120 == hz ) ) {
    return data;
  }


  std::vector<std::string> valvec;
  valvec.reserve( hz * 2 );

  std::stringstream stream( data );
  if ( 120 == hz ) {
    // remove every other value
    for ( std::string each; std::getline( stream, each, ',' ); valvec.push_back( each ) ) {
      std::getline( stream, each, ',' );
    }
  }
  else if ( 60 == hz ) {
    // keep one val, skip the next 3
    for ( std::string each; std::getline( stream, each, ',' ); valvec.push_back( each ) ) {
      std::getline( stream, each, ',' );
      std::getline( stream, each, ',' );
      std::getline( stream, each, ',' );
    }
  }

  std::string newvals;
  for ( auto s : valvec ) {
    if ( !newvals.empty( ) ) {

      newvals.append( "," );
    }
    newvals.append( s );
  }

  return newvals;
}

bool StpXmlReader::waveIsOk( const std::string& wavedata ) {
  // if all the values are -32768 or -32753, this isn't a valid reading
  bool oneok = false;
  std::stringstream stream( wavedata );
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    if ( !( "-32768" == each || "-32753" == each ) ) {
      return true;
    }
  }
  return false;
}
