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

#include "StpJsonReader.h"
#include "SignalData.h"
#include "DataRow.h"
#include "StreamChunkReader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

const std::string StpJsonReader::HEADER = "HEADER";
const std::string StpJsonReader::VITAL = "VITAL";
const std::string StpJsonReader::WAVE = "WAVE";
const std::string StpJsonReader::TIME = "TIME";

StpJsonReader::StpJsonReader( ) : Reader( "STPJSON" ), firstread( true ) {
}

StpJsonReader::StpJsonReader( const StpJsonReader& orig ) : Reader( orig ), firstread( orig.firstread ) {
}

StpJsonReader::~StpJsonReader( ) {
}

void StpJsonReader::finish( ) {
  stream->close( );
  stream.release( );
}

int StpJsonReader::prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) {
  int rslt = Reader::prepare( input, info );
  if ( 0 != rslt ) {
    return rslt;
  }

  firstread = true;

  bool usestdin = ( "-" == input || "-zl" == input );

  // zlib-compressed (first char='x'). Unfortunately, if we're reading from
  // stdin, we can't reset the stream back to the start, so we need to trust
  // that the user used the right switch
  if ( usestdin ) {
    stream.reset( new StreamChunkReader( &( std::cin ), ( "-zl" == input ), true ) );
  }
  else {
    // we need to read the first byte of the input stream to decide if it's 
    unsigned char firstbyte;
    std::ifstream * myfile = new std::ifstream( input, std::ios::binary );
    ( *myfile ) >> firstbyte;

    myfile->seekg( std::ios::beg ); // seek back to the beginning of the file
    stream.reset( new StreamChunkReader( myfile, ( 'x' == firstbyte ), false ) );
  }
  return 0;
}

ReadResult StpJsonReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& ) {
  // for this class we say a chunk is a full data set for one patient,
  // so read until we see another HEADER line in the text
  std::string onepatientdata = leftoverText + stream->readNextChunk( );
  ReadResult retcode = stream->rr;

  if ( ReadResult::ERROR == retcode ) {
    return retcode;
  }

  if ( ReadResult::NORMAL != retcode ) {
    // we read all the patient data we have, so process the results
    std::stringstream ss( onepatientdata );

    for ( std::string line; std::getline( ss, line, '\n' ); ) {
      handleOneLine( line, info );
    }
    leftoverText.clear( );

    return retcode;
  }

  size_t pos = onepatientdata.find( HEADER, HEADER.size( ) );
  firstread = false;
  while ( std::string::npos == pos ) {
    // no new HEADER line, so read some more
    std::string justread = stream->readNextChunk( );
    pos = onepatientdata.size( );
    onepatientdata += justread;
    retcode = stream->rr;

    if ( ReadResult::END_OF_FILE == retcode ) {
      break;
    }
    if ( ReadResult::ERROR == retcode ) {
      return retcode;
    }

    pos = onepatientdata.find( HEADER, pos );
  }

  // we either ran out of file, or we hit a HEADER line...figure out which
  if ( ReadResult::NORMAL == retcode ) {
    // we hit a new HEADER
    retcode = ReadResult::END_OF_PATIENT;
  }


  if ( retcode != ReadResult::ERROR ) {
    std::stringstream ss( onepatientdata.substr( 0, pos ) );

    for ( std::string line; std::getline( ss, line, '\n' ); ) {
      handleOneLine( line, info );
    }

    leftoverText = onepatientdata.substr( pos );
  }

  firstread = false;
  return retcode;
}

void StpJsonReader::handleOneLine( const std::string& chunk, std::unique_ptr<SignalSet>& info ) {
  if ( HEADER == chunk ) {
    state = jsonReaderState::JIN_HEADER;
  }
  else {
    const int pos = chunk.find( ' ' );
    const std::string firstword = chunk.substr( 0, pos );
    if ( VITAL == firstword ) {
      state = jsonReaderState::JIN_VITAL;

      std::stringstream points( chunk.substr( pos + 1 ) );
      std::string vital;
      std::string uom;
      std::string val;
      std::string high;
      std::string low;

      std::getline( points, vital, '|' );
      std::getline( points, uom, '|' );
      std::getline( points, val, '|' );
      std::getline( points, high, '|' );
      std::getline( points, low, '|' );

      bool added;
      std::unique_ptr<SignalData>& dataset = info->addVital( vital, &added );

      if ( val.empty( ) ) {
        output( ) << "empty val? " << chunk << std::endl;
      }

      if ( added ) {
        dataset->setUom( uom );
        dataset->setChunkIntervalAndSampleRate( 2000, 1 );
      }
      DataRow row( currentTime, val, high, low );
      dataset->add( row );
    }
    else if ( WAVE == firstword ) {
      state = jsonReaderState::JIN_WAVE;
      std::stringstream points( chunk.substr( pos + 1 ) );
      std::string wavename;
      std::string uom;
      std::string val;
      std::getline( points, wavename, '|' );
      std::getline( points, uom, '|' );
      std::getline( points, val, '|' );

      bool first;
      std::unique_ptr<SignalData>& dataset = info->addWave( wavename, &first );
      points >> wavename >> uom >> val;

      if ( first ) {
        dataset->setChunkIntervalAndSampleRate( 2000, 480 );
        dataset->setUom( uom );
      }

      DataRow row( currentTime, val );
      dataset->add( row );
    }
    else if ( TIME == firstword ) {
      state = jsonReaderState::JIN_TIME;
      currentTime = std::stoi( chunk.substr( pos + 1 ) );
    }
    else if ( state == jsonReaderState::JIN_HEADER ) {
      const int epos = chunk.find( '=' );
      std::string key = chunk.substr( 0, epos );
      std::string val = chunk.substr( epos + 1 );
      info->setMeta( key, val );
    }
  }
}
