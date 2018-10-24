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

#include "ZlReader2.h"
#include "SignalData.h"
#include "DataRow.h"
#include "Hdf5Writer.h"
#include "StreamChunkReader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <experimental/filesystem>
#include "config.h"
#include "json.hpp"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

namespace fs = std::experimental::filesystem::v1;

SaxJson::SaxJson( std::unique_ptr<SignalData>& signl ) : signal( signl ) {
}

bool SaxJson::start_array( std::size_t elements ) {
  data.clear( );
  return true;
}

bool SaxJson::end_array( ) {
  if ( 3 == data.size( ) ) {
    signal->add( DataRow( std::stol( data[0] ), data[2] ) );
  }
  return true;
}

bool SaxJson::null( ) {
  return true;
}

// called when a boolean is parsed; value is passed

bool SaxJson::boolean( bool val ) {
  return true;
}

// called when a signed or unsigned integer number is parsed; value is passed

bool SaxJson::number_integer( number_integer_t val ) {
  return true;
}

bool SaxJson::number_unsigned( number_unsigned_t val ) {
  return true;
}

// called when a floating-point number is parsed; value and original string is passed

bool SaxJson::number_float( number_float_t val, const string_t& s ) {
  return true;
}

// called when a string is parsed; value is passed and can be safely moved away

bool SaxJson::string( string_t& val ) {
  data.push_back( val );
  return true;
}

// called when an object or array begins or ends, resp. The number of elements is passed (or -1 if not known)

bool SaxJson::start_object( std::size_t elements ) {
  return true;
}

bool SaxJson::end_object( ) {
  return true;
}
// called when an object key is parsed; value is passed and can be safely moved away

bool SaxJson::key( string_t& val ) {
  return true;
}

// called when a parse error occurs; byte position, the last token, and an exception is passed

bool SaxJson::parse_error( std::size_t position, const std::string& last_token, const nlohmann::detail::exception& ex ) {
  return true;
}

ZlReader2::ZlReader2( ) : Reader( "Zl" ), firstread( true ) {
}

ZlReader2::ZlReader2( const std::string& name ) : Reader( name ), firstread( true ) {
}

ZlReader2::ZlReader2( const ZlReader2& orig ) : Reader( orig ), firstread( orig.firstread ) {
}

ZlReader2::~ZlReader2( ) {
}

void ZlReader2::finish( ) {
  signalToReaderLkp.clear( );
}

size_t ZlReader2::getSize( const std::string& input ) const {
  struct stat info;

  if ( stat( input.c_str( ), &info ) < 0 ) {
    perror( input.c_str( ) );
    return 0;
  }

  return info.st_size;
}

int ZlReader2::prepare( const std::string& input, std::unique_ptr<SignalSet>& data ) {
  int rslt = Reader::prepare( input, data );
  if ( rslt != 0 ) {
    return rslt;
  }

  // the new format is a directory full of gzip-compressed files
  fs::path p1 = input;
  if ( !fs::is_directory( p1 ) ) {
    return -1;
  }

  signalToReaderLkp.clear( );
  signalToParserLkp.clear( );
  for ( const auto& path : fs::directory_iterator( p1 ) ) {
    std::string filename( path.path( ).generic_string( ) );

    size_t lastdash = filename.find_last_of( '-' ) + 1;
    size_t ext = filename.find_last_of( '.' );
    std::string signal( filename.substr( lastdash, ext - lastdash ) );

    // we need to read the first byte of the input stream to decide if it's compressed
    unsigned char firstbyte;
    unsigned char secondbyte;
    std::ifstream * myfile = new std::ifstream( filename, std::ios::binary );
    ( *myfile ) >> firstbyte;
    ( *myfile ) >> secondbyte;

    bool islibz = ( 0x78 == firstbyte );
    bool isgz = ( 0x1F == firstbyte && 0x8B == secondbyte );

    myfile->seekg( std::ios::beg ); // seek back to the beginning of the file
    signalToReaderLkp[signal] = std::unique_ptr<StreamChunkReader>( new StreamChunkReader( myfile,
        ( islibz || isgz ), false, isgz, 128 ) );
  }

  return 0;
}

ReadResult ZlReader2::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {
  // we need to loop through all our signals to fill the SignalSet

  // the format looks like this:
  // [
  //  [
  //    "1519448400229",
  //    "1010068",
  //    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"
  //  ],
  //  [
  //    "1519448400478",                  <-- sample time (ms)
  //    "1010069",                        <-- sequence number (ignored)
  //    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"   <-- readings
  //  ],
  //  ...
  // ]

  for ( const auto& x : signalToReaderLkp ) {
    if ( 0 == signalToParserLkp.count( x.first ) ) {
      // haven't seen this signal before, so make a new parser for it
      signalToParserLkp[x.first]
          = std::unique_ptr<SaxJson>( new SaxJson( info->addWave( x.first ) ) );
    }

    std::string data = normalizeText( x.first, x.second->readNextChunk( ) );
    std::cout << data << std::endl;

    std::unique_ptr<SaxJson>& parser = signalToParserLkp.at( x.first );
    if ( "" != data ) {
      nlohmann::json::sax_parse( data, parser.get( ) );
    }

    while ( ReadResult::NORMAL == x.second->rr ) {
      data = normalizeText( x.first, x.second->readNextChunk( ) );
      if ( "" != data ) {
        std::cout << data << std::endl;
        std::cout << leftovers[x.first] << std::endl;

        nlohmann::json::sax_parse( data, parser.get( ) );
      }
    }
  }

  // for this class we say a chunk is a full data set for one patient,
  // so read until we see another HEADER line in the text
  //  ReadResult retcode = stream->rr;
  //
  //  if ( ReadResult::ERROR == retcode ) {
  //    return retcode;
  //  }
  //
  //  if ( ReadResult::NORMAL != retcode ) {
  //    // we read all the patient data we have, so process the results
  //    return retcode;
  //  }
  //
  //  firstread = false;
  //  return retcode;
  return ReadResult::END_OF_FILE;
}

std::string ZlReader2::normalizeText( const std::string& signalname, std::string text ) {
  std::string data = leftovers[signalname] + text;
  size_t lastclose = data.find_last_of( ']' );
  if ( std::string::npos == lastclose ) {
    // no ending array marker, so _everything_ goes to leftovers  
    leftovers[signalname] += data;
    return "";
  }
  else {
    // we have a ], so sheer off the stuff after it, and return
    leftovers[signalname] = data.substr( lastclose + 1 );
    return data.substr( 0, lastclose );
  }
}
