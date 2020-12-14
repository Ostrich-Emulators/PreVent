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
#include "Log.h"

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

namespace FormatConverter{
  const int StreamChunkReader::DEFAULT_CHUNKSIZE = 1024 * 256;

  std::unique_ptr<StreamChunkReader> StreamChunkReader::fromStdin( StreamType t, int chunksize ) {
    return std::unique_ptr<StreamChunkReader>( new StreamChunkReader( &( std::cin ), false, t, chunksize ) );
  }

  std::unique_ptr<StreamChunkReader> StreamChunkReader::fromFile( const std::string& filename,
      int chunksize ) {
    std::ifstream * myfile = new std::ifstream( filename, std::ios::in );

    // we need to read the first byte of the input stream to decide if it's compressed
    unsigned char firstbyte;
    unsigned char secondbyte;

    ( *myfile ) >> firstbyte;
    ( *myfile ) >> secondbyte;
    myfile->seekg( std::ios::beg ); // seek back to the beginning of the file

    StreamType t = StreamType::RAW;
    if ( 0x78 == firstbyte ) {
      t = StreamType::COMPRESS;
    }
    else if ( 0x1F == firstbyte && 0x8B == secondbyte ) {
      t = StreamType::GZIP;
    }
    else if ( 0x50 == firstbyte && 0x4B == secondbyte ) {
      t = StreamType::ZIP;
      return std::unique_ptr<StreamChunkReader>( new StreamChunkReader( filename, t, chunksize ) );
    }

    return std::unique_ptr<StreamChunkReader>( new StreamChunkReader( myfile, true, t, chunksize ) );
  }

  StreamChunkReader::StreamChunkReader( std::istream * cin, bool freeit,
      StreamType stype, int chunksz ) : rr( ReadResult::NORMAL ),
      closeStreamAtEnd( freeit ), chunksize( chunksz ), type( stype ), stream( cin ) {

    if ( StreamType::COMPRESS == type || StreamType::GZIP == type ) {
      initZlib( StreamType::GZIP == type );
    }
    else if ( StreamType::ZIP == type ) {
      Log::error( ) << "cannot open zip archives from stdin" << std::endl;
      throw std::runtime_error( "cannot open zip archives from stdin" );
    }
  }

  StreamChunkReader::StreamChunkReader( const std::string& filename,
      StreamType stype, int chunksz ) : rr( ReadResult::NORMAL ),
      closeStreamAtEnd( true ), chunksize( chunksz ), type( stype ) {

    if ( StreamType::COMPRESS == type || StreamType::GZIP == type ) {
      stream = new std::ifstream( filename, std::ios::in );
      initZlib( StreamType::GZIP == type );
    }
    else if ( StreamType::ZIP == type ) {
      Log::info( ) << "input is zip stream" << std::endl;
      int errorp = 0;
      ziparchive = zip_open( filename.c_str( ), ZIP_RDONLY, &errorp );
      if ( 0 != errorp ) {
        Log::error( ) << "error opening file: " << filename << std::endl;
      }

      // NOTE: we only expect a single file in the zip archive
      zipdata = zip_fopen_index( ziparchive, 0, 0 );
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
      Log::error( ) << "zlib error code: " << ret << std::endl;
      exit( 0 );
    }
  }

  void StreamChunkReader::setChunkSize( int size ) {
    chunksize = size;
  }

  void StreamChunkReader::close( ) {
    if ( StreamType::COMPRESS == type || StreamType::GZIP == type ) {
      inflateEnd( &strm );
    }

    if ( closeStreamAtEnd ) {
      if ( StreamType::COMPRESS == type || StreamType::GZIP == type ) {
        std::ifstream * str = static_cast<std::ifstream *> ( stream );
        if ( str->is_open( ) ) {
          str->close( );
        }
      }
      else if ( StreamType::ZIP == type ) {
        if ( nullptr != zipdata ) {
          zip_fclose( zipdata );
          zipdata = nullptr;
        }

        if ( nullptr != ziparchive ) {
          zip_discard( ziparchive );
          ziparchive = nullptr;
        }
      }
    }
  }

  std::string StreamChunkReader::readNextChunk( ) {
    return read( chunksize );
  }

  int StreamChunkReader::read( std::vector<char>& vec, int numbytes ) {
    if ( StreamType::ZIP == type ) {
      auto read = zip_fread( zipdata, vec.data( ), numbytes );
      rr = ( read < 0
          ? ReadResult::END_OF_FILE
          : ReadResult::NORMAL );

      // if we didn't get a full read, we need to shrink the string
      vec.resize( read );
      return read;
    }
    else if ( StreamType::GZIP == type || StreamType::COMPRESS == type ) {
      auto data = readNextCompressedChunk( numbytes );
      vec.resize( data.length( ) );
      for ( size_t i = 0; i < data.length( ); i++ ) {
        vec[i] = data[i];
      }
      return data.size( );
    }
    else {
      // uncompressed stream
      if ( stream->good( ) ) {
        stream->read( vec.data( ), numbytes );
        int bytesread = stream->gcount( );
        return bytesread;
      }
    }
    return 0;
  }

  std::string StreamChunkReader::read( int bufsz ) {
    if ( StreamType::COMPRESS == type || StreamType::GZIP == type || StreamType::ZIP == type ) {
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

    if ( StreamType::COMPRESS == type || StreamType::GZIP == type ) {
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

    }
    else if ( StreamType::ZIP == type ) {
      // we're going to read directly into our output structure.
      // so make sure it's big enough to handle the data
      data.resize( bufsz );
      auto read = zip_fread( zipdata, data.data( ), bufsz );
      rr = ( read < 0
          ? ReadResult::END_OF_FILE
          : ReadResult::NORMAL );

      // if we didn't get a full read, we need to shrink the string
      data.resize( read );
    }

    return data;
  }
}
