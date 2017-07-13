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

ReadResult StpXmlReader::rslt = ReadResult::NORMAL;
std::string StpXmlReader::workingText;
std::string StpXmlReader::element;
std::string StpXmlReader::last;
const std::string StpXmlReader::MISSING_VALUESTR( "-32768" );
std::map<std::string, std::string> StpXmlReader::attrs;
DataRow StpXmlReader::current;
StpXmlReaderState StpXmlReader::state = StpXmlReaderState::OTHER;
time_t StpXmlReader::firsttime = 0;

StpXmlReader::StpXmlReader( ) {
  LIBXML_TEST_VERSION
}

StpXmlReader::StpXmlReader( const StpXmlReader& orig ) {
}

StpXmlReader::~StpXmlReader( ) {
}

void StpXmlReader::finish( ) {
  xmlFreeParserCtxt( context );
  stream->close( );
  stream.release( );
  context = NULL;
}

int StpXmlReader::getSize( const std::string& input ) const {
  struct stat info;

  if ( stat( input.c_str( ), &info ) < 0 ) {
    perror( input.c_str( ) );
    return -1;
  }

  return info.st_size;
}

int StpXmlReader::prepare( const std::string& input, ReadInfo& info ) {
  xmlSAXHandler handler = { NULL };
  handler.startDocument = &StpXmlReader::start;
  handler.endDocument = &StpXmlReader::finish;
  handler.startElement = &StpXmlReader::startElement;
  handler.endElement = &StpXmlReader::endElement;
  handler.characters = &StpXmlReader::chars;
  handler.error = &StpXmlReader::error;
  handler.fatalError = &StpXmlReader::error;

  stream.reset( new StreamChunkReader( new std::ifstream( input, std::ios::binary ),
      false, false ) );
  std::string data = stream->read( 4 );
  context = xmlCreatePushParserCtxt( &handler, &info, data.c_str( ),
      data.length( ), input.c_str( ) );
  return 0;
}

ReadResult StpXmlReader::readChunk( ReadInfo & info ) {
  std::string data = stream->readNextChunk( );
  if ( ReadResult::ERROR == stream->rr ) {
    return stream->rr;
  }

  xmlParseChunk( context, data.c_str( ), data.length( ),
      ( ReadResult::END_OF_FILE == stream->rr ? 1 : 0 ) );

  return rslt;
}

void StpXmlReader::start( void * ) {
  rslt = ReadResult::NORMAL;
}

void StpXmlReader::finish( void * ) {
  std::cout << "into end do2c" << std::endl;
}

void StpXmlReader::chars( void *, const xmlChar * ch, int len ) {
  std::string mychars( (char *) ch, len );

  // ltrim
  mychars.erase( mychars.begin( ), std::find_if( mychars.begin( ), mychars.end( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );

  // rtrim
  mychars.erase( std::find_if( mychars.rbegin( ), mychars.rend( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), mychars.end( ) );

  if ( !mychars.empty( ) ) {
    workingText += mychars;
  }
  rslt = ReadResult::NORMAL;
}

void StpXmlReader::startElement( void *, const xmlChar * name, const xmlChar ** attrarr ) {
  int idx = 0;
  attrs.clear( );
  if ( NULL != attrarr ) {
    char * key = (char *) attrarr[idx];
    while ( NULL != key ) {
      std::string val( (char *) attrarr[++idx] );
      attrs[std::string( key )] = val;
      key = (char *) attrarr[++idx];
    }
  }


  element.assign( (char *) name );
  if ( "FileInfo" == element || "PatientName" == element ) {
    state = StpXmlReaderState::HEADER;
  }
  else if ( "VitalSigns" == element ) {
    state = StpXmlReaderState::VITAL;
    time_t vtime = std::stol( attrs["Time"] );
    current.time = vtime;
    // check if time is a rollover event!
  }
  else if( "VS" == element ){
    current.data.clear( );
    current.high = MISSING_VALUESTR;
    current.low = MISSING_VALUESTR;
  }
  else if ( "Waveforms" == element ) {
    state = StpXmlReaderState::WAVE;
  }

  rslt = ReadResult::NORMAL;
}

void StpXmlReader::endElement( void * user_data, const xmlChar * name ) {
  // check for the header metadata of interest
  ReadInfo& info = convertUserDataToReadInfo( user_data );

  std::string element( (char *) name );
  bool cleartext = false;

  if ( StpXmlReaderState::HEADER == state ) {
    if ( "Filename" == element || "Unit" == element
        || "Bed" == element || "PatientName" == element ) {
      info.addMeta( element, workingText );
    }
    else if ( "FileInfo" == element ) {
      state = StpXmlReaderState::OTHER;
    }
    cleartext = true;
  }
  else if ( StpXmlReaderState::VITAL == state ) {
    rslt = handleVital( element, info );
    cleartext = true;
  }
  else if ( StpXmlReaderState::WAVE == state ) {
    rslt = handleWave( element, info );
    cleartext = true;
  }


  if ( cleartext ) {
    workingText.clear( );
  }
  rslt = ReadResult::NORMAL;
}

ReadResult StpXmlReader::handleWave( const std::string& element, ReadInfo& info ) {
  return ReadResult::NORMAL;
}

ReadResult StpXmlReader::handleVital( const std::string& element, ReadInfo& info ) {
  if ( "Par" == element ) {
    std::unique_ptr<SignalData>& sigdata = info.addVital( workingText );
    sigdata->setUom( "Uncalib" );
    last = workingText;
  }
  else if ( "Value" == element ) {
    current.data = workingText;

    if ( 0 != attrs.count( "UOM" ) ) {
      std::unique_ptr<SignalData>& sigdata = info.addVital( last );
      sigdata->setUom( attrs["UOM"] );
    }
  }
  else if ( "AlarmLimitLow" == element ) {
    current.low = workingText;
  }
  else if ( "AlarmLimitHigh" == element ) {
    current.high = workingText;
  }
  else if ( "VS" == element ) {
    std::unique_ptr<SignalData>& sigdata = info.addVital( last );
    sigdata->add( current );
    last.clear( );
  }
}

ReadInfo& StpXmlReader::convertUserDataToReadInfo( void * data ) {
  ReadInfo * dd = static_cast<ReadInfo *> ( data );
  return *dd;
}

void StpXmlReader::error( void *user_data, const char *msg, ... ) {
  std::cerr << "error: " << msg << std::endl;
  rslt = ReadResult::ERROR;
}
