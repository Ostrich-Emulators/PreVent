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

#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

const int ZlReader::CHUNKSIZE = 16384 * 16;
const std::string ZlReader::HEADER = "HEADER";
const std::string ZlReader::VITAL = "VITAL";
const std::string ZlReader::WAVE = "WAVE";
const std::string ZlReader::TIME = "TIME";

ZlReader::ZlReader( ) : firstheader( true ), firstread( true ) {
}

ZlReader::ZlReader( const ZlReader& orig ) : firstheader( orig.firstheader ),
firstread( orig.firstread ) {
}

ZlReader::~ZlReader( ) {
}

void ZlReader::finish( ) {
  stream->close( );
  stream.release( );
}

int ZlReader::getSize( const std::string& input ) const {
  struct stat info;

  if ( stat( input.c_str( ), &info ) < 0 ) {
    perror( input.c_str( ) );
    return -1;
  }

  return info.st_size;
}

int ZlReader::prepare( const std::string& input, ReadInfo& ) {
  firstheader = true;
  firstread = true;

  bool usestdin = ( "-" == input || "-zl" == input );

  // zlib-compressed (first char='x'). Unfortunately, if we're reading from
  // stdin, we can't reset the stream back to the start, so we need to trust
  // that the user used the right switch
  if ( usestdin ) {
    stream.reset( new ZlStream( &( std::cin ), ( "-zl" == input ), true ) );
  }
  else {
    // we need to read the first byte of the input stream to decide if it's 
    unsigned char firstbyte;
    std::ifstream * myfile = new std::ifstream( input, std::ios::binary );
    ( *myfile ) >> firstbyte;

    myfile->seekg( std::ios::beg ); // seek back to the beginning of the file
    stream.reset( new ZlStream( myfile, ( 'x' == firstbyte ), false ) );
  }
  return 0;
}

int ZlReader::readChunk( ReadInfo& info ) {

  std::string justread;
  int retcode = stream->readNextChunk( justread );

  if ( retcode >= 0 ) {
    // fill in the newly-read data
    // need to worry about a second HEADER line (which means new patient/day)
    // and stop filling info if we hit one
    handleInputChunk( justread, info );
  }

  firstread = false;
  return retcode;
}

void ZlReader::handleInputChunk( std::string& chunko, ReadInfo& info ) {
  // we don't know where our chunk ends, so add whatever left-overs we
  // have from the last read to this read, and then process line by line

  //std::cout << "work: " << workingText << std::endl << "chunk: " << chunko << std::endl;
  workingText += chunko;
  bool checkNewLines = true;

  // Check to see if we have a HEADER key somewhere in our new chunk
  // because we're going to break on HEADERs
  // start with position 1 because we don't want to hit the same HEADER twice
  size_t newheader = workingText.find( HEADER + "\n", 1 );

  std::string nextworkingtext;
  if ( std::string::npos != newheader ) {
    // we have another HEADER line in there, so chop the current read at that spot
    nextworkingtext = workingText.substr( newheader );
    workingText.erase( newheader ); // keep the newline

    // no need to check for newlines, because we're guaranteed to end in one now
    checkNewLines = false;
  }

  if ( checkNewLines ) {
    // if chunk ends with a newline, we're fine...but if not, split out the
    // text after the last newline

    size_t lastnewline = workingText.rfind( '\n' );
    if ( std::string::npos == lastnewline ) {
      // no newlines present...so add everything to our working text
      // (we still don't have anything to process at this point)
      return;
    }

    if ( workingText.length( ) != lastnewline ) {
      nextworkingtext = workingText.substr( lastnewline + 1 ); // +1: skip the newline char
      workingText.erase( lastnewline + 1 );
    }
    // else chunk ends with a newline
  }

  //std::cout<<workingText<<std::endl;
  //std::cout<<nextworkingtext<<std::endl;

  std::stringstream ss( workingText );

  for ( std::string line; std::getline( ss, line, '\n' ); ) {
    handleOneLine( line, info );
  }

  workingText = nextworkingtext;
}

void ZlReader::handleOneLine( const std::string& chunk, ReadInfo& info ) {
  if ( HEADER == chunk ) {
    state = zlReaderState::IN_HEADER;
    firstheader = false;
  }
  else {
    const int pos = chunk.find( ' ' );
    const std::string firstword = chunk.substr( 0, pos );
    if ( VITAL == firstword ) {
      state = zlReaderState::IN_VITAL;

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

      std::unique_ptr<SignalData>& dataset = info.addVital( vital );

      if ( val.empty( ) ) {
        std::cout << "empty val? " << chunk << std::endl;
      }

      int scale = DataRow::scale( val );
      if ( scale > dataset->scale( ) ) {
        dataset->setScale( scale );
      }
      dataset->setUom( uom );
      dataset->add( DataRow( currentTime, val, high, low ) );
    }
    else if ( WAVE == firstword ) {
      state = zlReaderState::IN_WAVE;
      std::stringstream points( chunk.substr( pos + 1 ) );
      std::string wavename;
      std::string uom;
      std::string val;
      std::getline( points, wavename, '|' );
      std::getline( points, uom, '|' );
      std::getline( points, val, '|' );

      std::unique_ptr<SignalData>& dataset = info.addWave( wavename );
      points >> wavename >> uom >> val;

      dataset->setUom( uom );
      dataset->add( DataRow( currentTime, val ) );
    }
    else if ( TIME == firstword ) {
      state = zlReaderState::IN_TIME;
      currentTime = std::stoi( chunk.substr( pos + 1 ) );
    }
    else if ( state == zlReaderState::IN_HEADER ) {
      const int epos = chunk.find( '=' );
      std::string key = chunk.substr( 0, epos );
      std::string val = chunk.substr( epos + 1 );
      info.addMeta( key, val );
    }
  }
}

ZlStream::ZlStream( std::istream * cin, bool compressed, bool isStdin )
: iscompressed( compressed ), usestdin( isStdin ), stream( cin ) {
  if ( iscompressed ) {
    initZlib( );
  }
}

ZlStream::~ZlStream( ) {
  close( );
}

void ZlStream::initZlib( ) {
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  int ret = inflateInit( &strm );

  if ( ret != Z_OK ) {
    std::cerr << "zlib error code: " << ret << std::endl;
    exit( 0 );
  }
}

void ZlStream::close( ) {
  if ( !usestdin ) {
    static_cast<std::ifstream *> ( stream )->close( );
  }
}

int cnt = 0;

int ZlStream::readNextChunk( std::string& data ) {
  if ( iscompressed ) {
    return readNextCompressedChunk( data );
  }
  else {
    // we're not dealing with compressed data, so just read in the text
    char in[ZlReader::CHUNKSIZE];

    if ( ( *stream ) ) {
      stream->read( in, ZlReader::CHUNKSIZE );
      int bytesread = stream->gcount( );
      cnt += bytesread;
      std::cout << "read " << bytesread << " for a total of: " << cnt << std::endl;
      data += std::string( (char *) in, bytesread );
      return 1;
    }
    return -1;
  }
}

int ZlStream::readNextCompressedChunk( std::string& data ) {
  unsigned char in[ZlReader::CHUNKSIZE];
  unsigned char out[ZlReader::CHUNKSIZE];

  int retcode = 0;
  stream->read( (char *) in, ZlReader::CHUNKSIZE );
  strm.avail_in = stream->gcount( );

  retcode = 1;
  if ( strm.avail_in == 0 ) {
    return 0;
  }

  strm.next_in = in;
  do {
    strm.avail_out = ZlReader::CHUNKSIZE;
    strm.next_out = out;
    retcode = inflate( &strm, Z_NO_FLUSH );
    assert( retcode != Z_STREAM_ERROR ); /* state not clobbered */
    switch ( retcode ) {
      case Z_NEED_DICT:
        retcode = Z_DATA_ERROR; /* and fall through */
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        retcode = -1;
    }
    int have = ZlReader::CHUNKSIZE - strm.avail_out;
    // okay, we have some data to look at
    std::string str( (char *) out, have );
    data += str;
  }
  while ( strm.avail_out == 0 );

  if ( retcode == Z_STREAM_END ) {
    retcode = 0;
  }
  else if ( retcode == Z_OK ) {
    retcode = 1;
  }

  return retcode;
}
