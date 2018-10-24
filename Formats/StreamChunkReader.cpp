/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StreamChunkReader.cpp
 * Author: ryan
 * 
 * Created on August 26, 2016, 12:55 PM
 * 
 * Almost all the zlib code was taken from http://www.zlib.net/zlib_how.html
 */

#include "StreamChunkReader.h"

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

const int StreamChunkReader::DEFAULT_CHUNKSIZE = 1024 * 256;

StreamChunkReader::StreamChunkReader( std::istream * cin, bool compressed,
    bool isStdin, bool isGzip, int chunksz ) : rr( ReadResult::NORMAL ),
iscompressed( compressed ), usestdin( isStdin ), chunksize( chunksz ), stream( cin ) {
  if ( iscompressed ) {
    initZlib( isGzip );
  }
}

StreamChunkReader::~StreamChunkReader( ) {
  close( );
}

void StreamChunkReader::initZlib( bool forGzip ) {
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;

  int ret = ( forGzip
      ? inflateInit2( &strm, 16 + MAX_WBITS )
      : inflateInit( &strm ) );

  if ( ret != Z_OK ) {
    std::cerr << "zlib error code: " << ret << std::endl;
    exit( 0 );
  }
}

void StreamChunkReader::setChunkSize( int size ) {
  chunksize = size;
}

void StreamChunkReader::close( ) {
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

std::string StreamChunkReader::readNextChunk( ) {
  return read( chunksize );
}

int StreamChunkReader::read( std::vector<char>& vec, int numbytes ) {
  if ( stream->good( ) ) {
    stream->read( vec.data( ), numbytes );
    int bytesread = stream->gcount( );
    return bytesread;
  }
  return 0;
}

std::string StreamChunkReader::read( int bufsz ) {
  if ( iscompressed ) {
    return readNextCompressedChunk( bufsz );
  }
  else {
    // we're not dealing with compressed data, so just read in the text
    char * in[chunksize];
    //    std::cout << "g: " << stream->good( )
    //        << " b: " << stream->bad( )
    //        << " e: " << stream->eof( )
    //        << " f: " << stream->fail( ) << std::endl;
    if ( stream->good( ) ) {
      stream->read( (char *) in, bufsz );
      int bytesread = stream->gcount( );
      return std::string( (char *) in, bytesread );
    }
    rr = ReadResult::END_OF_FILE;
    return "";
  }
}

std::string StreamChunkReader::readNextCompressedChunk( int bufsz ) {
  unsigned char in[bufsz];
  unsigned char out[bufsz];

  std::string data;
  int retcode = 0;
  stream->read( (char *) in, bufsz );
  strm.avail_in = stream->gcount( );

  retcode = 1;
  if ( strm.avail_in == 0 ) {
    rr = ReadResult::END_OF_FILE;
    return "";
  }

  strm.next_in = in;
  do {
    strm.avail_out = bufsz;
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
    int have = bufsz - strm.avail_out;
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
