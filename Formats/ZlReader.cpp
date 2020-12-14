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

#include "ZlReader.h"
#include "SignalData.h"
#include "DataRow.h"
#include "Hdf5Writer.h"
#include "StreamChunkReader.h"
#include "Log.h"

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

namespace FormatConverter{

  const std::string ZlReader::HEADER = "HEADER";
  const std::string ZlReader::VITAL = "VITAL";
  const std::string ZlReader::WAVE = "WAVE";
  const std::string ZlReader::TIME = "TIME";

  ZlReader::ZlReader( ) : Reader( "Zl" ), firstread( true ) { }

  ZlReader::ZlReader( const std::string& name ) : Reader( name ), firstread( true ) { }

  ZlReader::ZlReader( const ZlReader& orig ) : Reader( orig ), firstread( orig.firstread ) { }

  ZlReader::~ZlReader( ) { }

  void ZlReader::finish( ) {
    stream->close( );
    stream.release( );
  }

  int ZlReader::prepare( const std::string& input, SignalSet * data ) {
    int rslt = Reader::prepare( input, data );
    if ( rslt != 0 ) {
      return rslt;
    }

    firstread = true;

    bool usestdin = ( "-" == input || "-zl" == input || "-gz" == input );

    // zlib-compressed (first char='x'). Unfortunately, if we're reading from
    // stdin, we can't reset the stream back to the start, so we need to trust
    // that the user used the right switch
    if ( usestdin ) {
      StreamType t = StreamType::RAW;
      if ( "-gz" == input ) {
        t = StreamType::GZIP;
      }
      else if ( "-lz" == input ) {
        t = StreamType::COMPRESS;
      }

      stream = StreamChunkReader::fromStdin( t );
    }
    else {
      // we're looking at an "old-skool" uva format, so we only have one file
      stream = StreamChunkReader::fromFile( input );
    }

    return 0;
  }

  ReadResult ZlReader::fill( SignalSet * info, const ReadResult& ) {
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
      retcode = ( splitter( ).nonbreaking( ) ? ReadResult::NORMAL : ReadResult::END_OF_PATIENT );
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

  void ZlReader::handleOneLine( const std::string& chunk, SignalSet * info ) {
    if ( HEADER == chunk ) {
      state = zlReaderState::ZIN_HEADER;
    }
    else {
      const int pos = chunk.find( ' ' );
      const std::string firstword = chunk.substr( 0, pos );
      if ( VITAL == firstword ) {
        state = zlReaderState::ZIN_VITAL;

        std::stringstream points( chunk.substr( pos + 1 ) );
        std::string vital;
        std::string uom;
        std::string val;

        std::getline( points, vital, '|' );
        std::getline( points, uom, '|' );
        std::getline( points, val, '|' );

        bool added;
        SignalData * dataset = info->addVital( vital, &added );

        if ( val.empty( ) ) {
          Log::debug( ) << "empty val? " << chunk << std::endl;
        }

        if ( added ) {
          dataset->setUom( uom );
          dataset->setChunkIntervalAndSampleRate( 2000, 1 );
        }
        dataset->add( DataRow::from( currentTime, val ) );
      }
      else if ( WAVE == firstword && !this->skipwaves( ) ) {
        state = zlReaderState::ZIN_WAVE;
        std::stringstream points( chunk.substr( pos + 1 ) );
        std::string wavename;
        std::string uom;
        std::string val;
        std::getline( points, wavename, '|' );
        std::getline( points, uom, '|' );
        std::getline( points, val, '|' );

        bool first;
        SignalData * dataset = info->addWave( wavename, &first );
        points >> wavename >> uom >> val;

        if ( first ) {
          dataset->setChunkIntervalAndSampleRate( 2000, 480 );
          dataset->setUom( uom );
        }

        dataset->add( DataRow::from( currentTime, val ) );
      }
      else if ( TIME == firstword ) {
        state = zlReaderState::ZIN_TIME;
        currentTime = modtime( std::stoi( chunk.substr( pos + 1 ) )* 1000 );
      }
      else if ( state == zlReaderState::ZIN_HEADER ) {
        const int epos = chunk.find( '=' );
        std::string key = chunk.substr( 0, epos );
        std::string val = chunk.substr( epos + 1 );
        info->setMeta( key, val );
      }
    }
  }
}