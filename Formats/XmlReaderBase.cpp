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

#include "XmlReaderBase.h"
#include "SignalData.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <map>
#include <memory>

const std::string XmlReaderBase::MISSING_VALUESTR( "-32768" );
bool XmlReaderBase::accumulateText = false;
std::string XmlReaderBase::working;
const int XmlReaderBase::READCHUNK = 16384 * 16;

XmlReaderBase::XmlReaderBase( const std::string& name ) : Reader( name ), datemodifier( 0 ) {
}

XmlReaderBase::XmlReaderBase( const XmlReaderBase& orig ) : Reader( orig ), datemodifier( 0 ) {
}

XmlReaderBase::~XmlReaderBase( ) {
}

void XmlReaderBase::finish( ) {
  XML_ParserFree( parser );
}

size_t XmlReaderBase::getSize( const std::string& input ) const {
  struct stat info;

  if ( stat( input.c_str( ), &info ) < 0 ) {
    perror( input.c_str( ) );
    return 0;
  }

  return info.st_size;
}

void XmlReaderBase::start( void * data, const char * el, const char ** attr ) {
  std::map<std::string, std::string> attrs;
  for ( int i = 0; attr[i]; i += 2 ) {
    attrs[attr[i]] = attr[i + 1];
  }

  XmlReaderBase * rdr = static_cast<XmlReaderBase *> ( data );
  rdr->start( el, attrs );
  accumulateText = true;
}

void XmlReaderBase::end( void * data, const char * el ) {
  XmlReaderBase * rdr = static_cast<XmlReaderBase *> ( data );
  rdr->end( el, trim( working ) );
  working.clear( );
  accumulateText = false;
}

void XmlReaderBase::chars( void * data, const char * text, int len ) {
  if ( accumulateText ) {
    working.append( text, len );
  }
}

std::string XmlReaderBase::trim( std::string & totrim ) {
  // ltrim
  totrim.erase( totrim.begin( ), std::find_if( totrim.begin( ), totrim.end( ),
        std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );

  // rtrim
  totrim.erase( std::find_if( totrim.rbegin( ), totrim.rend( ),
        std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), totrim.end( ) );

  return totrim;
}

void XmlReaderBase::startSaving( ) {
  saved.setMetadataFrom( *filler );
  filler = &saved;
}

void XmlReaderBase::setResult( ReadResult rslt ) {
  this->rslt = rslt;
}

int XmlReaderBase::prepare( const std::string& fname, SignalSet& info ) {
  int rr = Reader::prepare( fname, info );
  if ( 0 == rr ) {
    parser = XML_ParserCreate( NULL );
    XML_SetUserData( parser, this );
    XML_SetElementHandler( parser, XmlReaderBase::start, XmlReaderBase::end );
    XML_SetCharacterDataHandler( parser, XmlReaderBase::chars );
    input.open( fname, std::ifstream::in );
    setResult( ReadResult::NORMAL );
  }
  return rr;
}

void XmlReaderBase::copysaved( SignalSet& tgt ) {
  tgt.setMetadataFrom( saved );
  saved.metadata( ).clear( );

  for ( auto& m : saved.vitals( ) ) {
    std::unique_ptr<SignalData>& savedsignal = m.second;

    bool added = false;
    std::unique_ptr<SignalData>& infodata = tgt.addVital( m.first, &added );

    infodata->setMetadataFrom( *savedsignal );
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

    infodata->setMetadataFrom( *savedsignal );
    int rows = savedsignal->size( );
    for ( int row = 0; row < rows; row++ ) {
      const std::unique_ptr<DataRow>& datarow = savedsignal->pop( );
      infodata->add( *datarow );
    }
  }

  saved.vitals( ).clear( );
  saved.waves( ).clear( );
}

ReadResult XmlReaderBase::fill( SignalSet & info, const ReadResult& lastfill ) {
  if ( ReadResult::END_OF_DAY == lastfill || ReadResult::END_OF_PATIENT == lastfill ) {
    copysaved( info );
  }
  filler = &info;
  setResult( ReadResult::NORMAL );

  firstread = ( ReadResult::FIRST_READ == lastfill );

  std::vector<char> buffer( READCHUNK, 0 );
  while ( input.read( buffer.data( ), buffer.size( ) ) ) {
    long gcnt = input.gcount( );
    XML_Parse( parser, &buffer[0], buffer.size( ), gcnt < READCHUNK );
    if ( ReadResult::NORMAL != rslt ) {
      if ( ReadResult::END_OF_PATIENT == rslt && anonymizing( ) ) {
        info.metadata( )["Patient Name"] = "Anonymous";
      }

      return rslt;
    }
  }

  return ReadResult::END_OF_FILE;
}

bool XmlReaderBase::isRollover( const time_t& then, const time_t& now ) const {
  if ( 0 != then ) {
    time_t modnow = datemod( now );
    time_t modthen = datemod( then );
    
    const int cdoy = gmtime( &modnow )->tm_yday;
    const int pdoy = gmtime( &modthen )->tm_yday;
    if ( cdoy != pdoy ) {

      return true;
    }
  }

  return false;
}

time_t XmlReaderBase::time( const std::string& timer ) const {
  if ( std::string::npos == timer.find( " " )
        && std::string::npos == timer.find( "T" ) ) {
    return std::stol( timer );
  }

  // we have a local time that we need to convert
  std::string format = ( std::string::npos == timer.find( "T" )
        ? "%m/%d/%Y %I:%M:%S %p" // STPXML time string 
        : "%Y-%m-%dT%H:%M:%S" ); // CPC time string

  tm mytime;
  strptime( timer.c_str( ), format.c_str( ), &mytime );

  // now convert our local time to UTC
  time_t local = mktime( &mytime );
  mytime = *gmtime( &local );

  return mktime( &mytime );
}

bool XmlReaderBase::isFirstRead( ) const {
  return firstread;
}

void XmlReaderBase::setDateModifier( const time_t& mod ) {
  firstread = false;
  datemodifier = mod;
}

time_t XmlReaderBase::datemod( const time_t& rawdate ) const {
  time_t timer = ( rawdate - datemodifier );
  return timer;
}
