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

ZlReader::ZlReader( ) : firstread( true ) {
}

ZlReader::ZlReader( const ZlReader& orig ) : firstread( orig.firstread ) {
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

ReadResult ZlReader::readChunk( ReadInfo& info ) {
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

void ZlReader::handleOneLine( const std::string& chunk, ReadInfo& info ) {
  if ( HEADER == chunk ) {
    state = zlReaderState::IN_HEADER;
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
: iscompressed( compressed ), usestdin( isStdin ), stream( cin ), rr( ReadResult::NORMAL ) {
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
  if ( iscompressed ) {
    inflateEnd( &strm );
  }

  if ( !usestdin ) {
    std::ifstream * str = static_cast<std::ifstream *> ( stream );
    if ( str->is_open( ) ) {
      str->close( );
    }
  }
}

int cnt = 0;

std::string ZlStream::readNextChunk( ) {
  if ( iscompressed ) {
    return readNextCompressedChunk( );
  }
  else {
    // we're not dealing with compressed data, so just read in the text

    std::cout << "g: " << stream->good( )
        << " b: " << stream->bad( )
        << " e: " << stream->eof( )
        << " f: " << stream->fail( ) << std::endl;

    if ( stream->good( ) ) {
      stream->read( (char *) in, ZlReader::CHUNKSIZE );
      int bytesread = stream->gcount( );
      cnt += bytesread;
      std::cout << "read " << bytesread << " for a total of: " << cnt << std::endl;
      return std::string( (char *) in, bytesread );
    }
    rr = ReadResult::END_OF_FILE;
    return "";
  }
}

std::string ZlStream::readNextCompressedChunk( ) {
  unsigned char out[ZlReader::CHUNKSIZE];

  std::string data;
  int retcode = 0;
  stream->read( (char *) in, ZlReader::CHUNKSIZE );
  strm.avail_in = stream->gcount( );

  retcode = 1;
  if ( strm.avail_in == 0 ) {
    rr = ReadResult::END_OF_FILE;
    return "";
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
        rr = ReadResult::ERROR;
        inflateEnd( &strm );
    }
    int have = ZlReader::CHUNKSIZE - strm.avail_out;
    // okay, we have some data to look at
    std::string str( (char *) out, have );
    data += str;
  }
  while ( strm.avail_out == 0 );

  if ( retcode == Z_STREAM_END ) {
    rr = ReadResult::END_OF_FILE;
  }
  else if ( retcode == Z_OK ) {
    rr = ReadResult::NORMAL;
  }

  return data;
}
