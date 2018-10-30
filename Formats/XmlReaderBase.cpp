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
#include "StreamChunkReader.h"
#include "BasicSignalSet.h"

#include <ctime>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <map>
#include <memory>

bool XmlReaderBase::accumulateText = false;
std::string XmlReaderBase::working;
const int XmlReaderBase::READCHUNK = 1024 * 1024;

XmlReaderBase::XmlReaderBase( const std::string& name ) : Reader( name ), filler( nullptr ) {
  time_t reftime = std::time( nullptr );
  tm * reftm = localtime( &reftime );
  tz_name = reftm->tm_zone;
  tz_offset = reftm->tm_gmtoff;
}

XmlReaderBase::XmlReaderBase( const XmlReaderBase& orig ) : Reader( orig ), filler( nullptr ) {
  time_t reftime = std::time( nullptr );
  tm * reftm = localtime( &reftime );
  tz_name = reftm->tm_zone;
  tz_offset = reftm->tm_gmtoff;
}

XmlReaderBase::~XmlReaderBase( ) {
}

void XmlReaderBase::finish( ) {
  XML_ParserFree( parser );
  input->close( );
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

void XmlReaderBase::startSaving( dr_time savetime ) {
  if ( savetime != lastSaveTime ) {
    saved.setMetadataFrom( *filler );
    filler = &saved;
    filler->clearOffsets( );
    lastSaveTime = savetime;
  }
}

void XmlReaderBase::setResult( ReadResult rslt ) {
  // if we're not breaking our output, then ignore End of Day and End of Patient
  if ( nonbreaking( ) &&
      ( ReadResult::END_OF_DAY == rslt || ReadResult::END_OF_PATIENT == rslt ) ) {
    return;
  }
  this->rslt = rslt;
}

int XmlReaderBase::prepare( const std::string& fname, std::unique_ptr<SignalSet>& info ) {
  int rr = Reader::prepare( fname, info );
  if ( 0 == rr ) {
    parser = XML_ParserCreate( NULL );
    XML_SetUserData( parser, this );
    XML_SetElementHandler( parser, XmlReaderBase::start, XmlReaderBase::end );
    XML_SetCharacterDataHandler( parser, XmlReaderBase::chars );
    XML_SetCommentHandler( parser, XmlReaderBase::comment );

    if ( "-" == fname ) {
      input.reset( new StreamChunkReader( &( std::cin ), false, true, READCHUNK ) );
    }
    else {
      std::ifstream * myfile = new std::ifstream( fname, std::ios::in );
      input.reset( new StreamChunkReader( myfile, false, false, READCHUNK ) );
    }
  }
  return rr;
}

void XmlReaderBase::comment( void* data, const char* text ) {
  XmlReaderBase * rdr = static_cast<XmlReaderBase *> ( data );
  std::string comment( text );
  rdr->comment( trim( comment ) );
}

void XmlReaderBase::copySavedInto( std::unique_ptr<SignalSet>& tgt ) {
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

ReadResult XmlReaderBase::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastfill ) {
  if ( ReadResult::END_OF_DAY == lastfill || ReadResult::END_OF_PATIENT == lastfill ) {
    copySavedInto( info );
  }
  filler = info.get( );
  setResult( ReadResult::NORMAL );

  std::vector<char> buffer( READCHUNK, 0 );
  int bytesread = input->read( buffer, READCHUNK );
  while ( 0 != bytesread ) {
    // output( ) << buffer.data( ) << std::endl;
    bool waslast = ( bytesread < READCHUNK );
    XML_Status status = XML_Parse( parser, buffer.data( ), bytesread, waslast );
    if ( status != XML_STATUS_OK ) {
      XML_Error err = XML_GetErrorCode( parser );
      std::cerr << XML_ErrorString( err )
          << " line: " << XML_GetCurrentLineNumber( parser )
          << " column: " << XML_GetCurrentColumnNumber( parser )
          << std::endl;
      return ReadResult::ERROR;
    }
    if ( ReadResult::NORMAL != rslt ) {
      return rslt;
    }

    bytesread = input->read( buffer, READCHUNK );
  }

  return ReadResult::END_OF_FILE;
}

bool XmlReaderBase::isRollover( const dr_time& then, const dr_time& now ) const {
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

dr_time XmlReaderBase::time( const std::string& timer, bool valIsLocal ) const {
  if ( std::string::npos == timer.find( " " )
      && std::string::npos == timer.find( "T" ) ) {
    // no space and no T---we must have a unix timestamp
    // and we believe Stp saves unix timestamps in UTC always
    // (regardless of valIsLocal)
    return std::stol( timer ) * 1000;
  }

  // we have a local time that we need to convert
  std::string format = ( std::string::npos == timer.find( "T" )
      ? "%m/%d/%Y %I:%M:%S %p" // STPXML time string
      : "%Y-%m-%dT%H:%M:%S" ); // CPC time string

  tm mytime = { 0, };
  
  strptime2( timer, format, &mytime );
  //output( ) << mytime.tm_hour << ":" << mytime.tm_min << ":" << mytime.tm_sec << std::endl;

  // now convert our local time to UTC
  if ( valIsLocal ) {
    mytime.tm_zone = tz_name.c_str( );
    mytime.tm_gmtoff = tz_offset;

    time_t local = mktime( &mytime );
    mytime = *gmtime( &local );
    //output( ) << mytime.tm_hour << ":" << mytime.tm_min << ":" << mytime.tm_sec << std::endl;
  }

  return mktime( &mytime )* 1000; // convert seconds to ms
}
