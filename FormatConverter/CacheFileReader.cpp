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

#include "CacheFileReader.h"
#include "DataSetDataCache.h"
#include "DataRow.h"
#include "Hdf5Writer.h"

#include <zlib.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <cstdio>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

const int CacheFileReader::CHUNKSIZE = 16384 * 16;
const std::string CacheFileReader::HEADER = "HEADER";
const std::string CacheFileReader::VITAL = "VITAL";
const std::string CacheFileReader::WAVE = "WAVE";
const std::string CacheFileReader::TIME = "TIME";

CacheFileReader::CacheFileReader( const std::string& output,
        int _compression, bool _bigfile, const std::string& _prefix )
: outputdir( output ), compression( _compression ), largefile( _bigfile ), ordinal( 1 ),
prefix( _prefix ), firstheader( true ) {
}

CacheFileReader::CacheFileReader( const CacheFileReader& orig )
: outputdir( orig.outputdir ), compression( orig.compression ),
largefile( orig.largefile ), ordinal( orig.ordinal ), firstheader( orig.firstheader ) {
}

CacheFileReader::~CacheFileReader( ) {
}

int CacheFileReader::convert( const std::string& input ) {
  bool usestdin = ( "-" == input || "-zl" == input );

  // zlib-compressed (first char='x'). Unfortunately, if we're reading from
  // stdin, we can't reset the stream back to the start, so we need to account 
  // for this one extra btye.
  if ( usestdin ) {
    return convert( std::cin, "-zl" == input );
  }
  else {
    // we need to read the first byte of the input stream to decide if it's 
    unsigned char firstbyte;
    std::ifstream myfile( input, std::ios::binary );
    myfile >> firstbyte;
    myfile.seekg( std::ios::beg ); // seek back to the beginning of the file
    int ret = convert( myfile, 'x' == firstbyte );
    myfile.close( );
    return ret;
  }
}

int CacheFileReader::convert( std::istream& stream, bool compressed ) {
  reset( );
  firstheader = true;

  if ( compressed ) {
    return convertcompressed( stream );
  }

  // if we're not dealing with compressed data, then just read the
  // bytes (they're text) and handle the strings as they come
  char in[CHUNKSIZE];

  while ( stream ) {
    stream.read( in, CHUNKSIZE );
    int bytesread = stream.gcount( );
    std::string str( (char *) in, bytesread );
    handleInputChunk( str );
  }
  flush( );

  return 0;
}

int CacheFileReader::convertcompressed( std::istream& stream ) {
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNKSIZE];
  unsigned char out[CHUNKSIZE];

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit( &strm );
  if ( ret != Z_OK ) {
    std::cerr << "zlib error code: " << ret << std::endl;
    return 1;
  }

  while ( stream && ret != Z_STREAM_END ) {
    stream.read( (char *) in, CHUNKSIZE );
    strm.avail_in = stream.gcount( );

    if ( strm.avail_in == 0 ) {
      break;
    }

    strm.next_in = in;
    do {
      strm.avail_out = CHUNKSIZE;
      strm.next_out = out;
      ret = inflate( &strm, Z_NO_FLUSH );
      assert( ret != Z_STREAM_ERROR ); /* state not clobbered */
      switch ( ret ) {
        case Z_NEED_DICT:
          ret = Z_DATA_ERROR; /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          (void) inflateEnd( &strm );
          return -1;
      }
      have = CHUNKSIZE - strm.avail_out;
      // okay, we have some data to look at
      std::string str( (char *) out, have );
      handleInputChunk( str );
    } while ( strm.avail_out == 0 );
  }

  (void) inflateEnd( &strm );

  flush( );
  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;

}

void CacheFileReader::handleInputChunk( std::string& chunk ) {
  // we don't know where our chunk ends, so add whatever left-overs we
  // have from the last read to this read, and then process line by line

  // if chunk ends with a newline, we're fine...but if not, split out the
  // text after the last newline

  size_t lastnewline = chunk.rfind( '\n' );
  if ( std::string::npos == lastnewline ) {
    // no newlines present...so add everything to our working text
    workingText += chunk;
    return;
  }

  std::string nextworkingtext;
  if ( chunk.length( ) != lastnewline ) {
    nextworkingtext = chunk.substr( lastnewline + 1 ); // +1: skip the newline char
    chunk.erase( lastnewline + 1 );
  }
  // else chunk ends with a newline

  std::stringstream ss( workingText + chunk );

  for ( std::string line; std::getline( ss, line, '\n' ); ) {
    handleOneLine( line );
  }

  workingText = nextworkingtext;
}

void CacheFileReader::reset( ) {
  lastTime = 0;
  firstTime = 2099999999;
  vitals.clear( );
  waves.clear( );
}

void CacheFileReader::flush( ) {
  Hdf5Writer::flush( outputdir, prefix, compression, firstTime, lastTime,
          ordinal, datasetattrs, vitals, waves );
  reset( );
}

void CacheFileReader::handleOneLine( const std::string& chunk ) {
  if ( HEADER == chunk ) {
    state = zlReaderState::IN_HEADER;
    if ( firstheader ) {
      firstheader = false;
    }
    else {
      flush( );
      std::cout << "new header found...rolling over" << std::endl;
    }
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

      if ( 0 == vitals.count( vital ) ) {
        vitals.insert( std::make_pair( vital,
                std::unique_ptr<DataSetDataCache>( new DataSetDataCache( vital,
                largefile ) ) ) );
      }

      int scale = DataRow::scale( val );
      if ( scale > vitals[vital]->scale( ) ) {
        vitals[vital]->setScale( scale );
      }
      vitals[vital]->setUom( uom );
      vitals[vital]->add( DataRow( currentTime, val, high, low ) );
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

      points >> wavename >> uom >> val;
      if ( 0 == waves.count( wavename ) ) {
        waves.insert( std::make_pair( wavename,
                std::unique_ptr<DataSetDataCache>( new DataSetDataCache( wavename,
                largefile ) ) ) );
      }

      waves[wavename]->setUom( uom );
      waves[wavename]->add( DataRow( currentTime, val ) );
    }
    else if ( TIME == firstword ) {
      state = zlReaderState::IN_TIME;
      currentTime = std::stoi( chunk.substr( pos + 1 ) );

      if ( currentTime < firstTime ) {
        firstTime = currentTime;
      }
      if ( currentTime > lastTime ) {
        lastTime = currentTime;
      }
    }
    else if ( state == zlReaderState::IN_HEADER ) {
      const int epos = chunk.find( '=' );
      std::string key = chunk.substr( 0, epos );
      std::string val = chunk.substr( epos + 1 );

      //check patient names
      if ( "Patient Name" == key && 0 != datasetattrs.count( key ) ) {
        if ( datasetattrs[key] != val ) {
          std::cout << "WARNING: two patients in this datafile" << std::endl;
          ordinal++;
        }
      }

      datasetattrs[key] = val;
    }
  }
}
