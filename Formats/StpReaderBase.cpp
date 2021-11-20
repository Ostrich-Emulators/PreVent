/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "StpReaderBase.h"
#include "SignalData.h"
#include "DataRow.h"
#include "Hdf5Writer.h"
#include "StreamChunkReader.h"
#include "BasicSignalSet.h"
#include "Log.h"

#include <sys/stat.h>
#include <filesystem>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

namespace FormatConverter {

  StpReaderBase::StpReaderBase( const std::string& name ) : Reader( name ),
      work( 1024 * 1024 * 16 ), metadataonly( false ) {
  }

  StpReaderBase::StpReaderBase( const StpReaderBase& orig ) : Reader( orig ),
      work( orig.work.capacity( ) ), metadataonly( orig.metadataonly ) {
  }

  StpReaderBase::~StpReaderBase( ) {
  }

  void StpReaderBase::setMetadataOnly( bool metasonly ) {
    metadataonly = metasonly;
  }

  bool StpReaderBase::isMetadataOnly( ) const {
    return metadataonly;
  }

  int StpReaderBase::prepare( const std::string& filename, SignalSet * data ) {
    int rslt = Reader::prepare( filename, data );
    if ( rslt != 0 ) {
      return rslt;
    }

    // STP files are a concatenation of zlib-compressed segments, so we need to
    // know when we're *really* done reading. Otherwise, the decoder will say
    // it's at the end of the file once the first segment is fully read
    struct stat filestat;
    if ( stat( filename.c_str( ), &filestat ) == -1 ) {
      perror( "stat failed" );
      return -1;
    }

//    zstr::ifstream doublereader( filename );
//    char * skipper = new char[1024 * 1024];
//    auto path = std::filesystem::path{ filename };
//    path.replace_extension( "segs" );
//    std::ofstream outy( path.filename( ) );
//    while ( true ) {
//      doublereader.read( skipper, 1024 * 1024 );
//      auto g = doublereader.gcount( );
//      if ( 0 == g ) {
//        break;
//      }
//      outy.write( skipper, g );
//    }
//
//    outy.close( );
//    delete []skipper;

    filestream.open( filename, std::ifstream::in | std::ifstream::binary );
    zipstream = new zstr::istream( filestream.rdbuf( ) );
    return 0;
  }

  void StpReaderBase::finish( ) {
    if ( zipstream ) {
      delete zipstream;
    }
    if ( filestream.is_open( ) ) {
      filestream.close( );
    }
  }

  unsigned long StpReaderBase::popUInt64( ) {
    std::vector<unsigned char> vec = work.popvec( 4 );
    return ( vec[0] << 24 | vec[1] << 16 | vec[2] << 8 | vec[3] );
  }

  unsigned int StpReaderBase::popUInt16( ) {
    unsigned char b1 = work.pop( );
    unsigned char b2 = work.pop( );
    return ( b1 << 8 | b2 );
  }

  unsigned int StpReaderBase::readUInt16( ) {
    unsigned char b1 = work.read( );
    unsigned char b2 = work.read( 1 );
    return ( b1 << 8 | b2 );
  }

  int StpReaderBase::popInt16( ) {
    unsigned char b1 = work.pop( );
    unsigned char b2 = work.pop( );

    short val = ( b1 << 8 | b2 );
    return val;
  }

  int StpReaderBase::popInt8( ) {
    return (char) work.pop( );
  }

  unsigned int StpReaderBase::popUInt8( ) {
    return work.pop( );
  }

  std::string StpReaderBase::popString( size_t length ) {
    std::vector<char> chars;
    chars.reserve( length );
    auto data = work.popvec( length );
    for ( size_t i = 0; i < data.size( ) && 0 != data[i]; i++ ) {
      chars.push_back( (char) data[i] );
    }
    return std::string( chars.begin( ), chars.end( ) );
  }

  std::string StpReaderBase::readString( size_t length ) {
    std::vector<char> chars;
    chars.reserve( length );

    for ( size_t i = 0; i < length; i++ ) {
      chars.push_back( (char) work.read( i ) );
    }
    return std::string( chars.begin( ), chars.end( ) );
  }

  int StpReaderBase::readMore( ) {
    std::vector<unsigned char> decodebuffer;
    decodebuffer.reserve( 1024 * 1024 * 4 );

    bool readsome = false;
    while ( !readsome ) {
      try {
        zipstream->read( (char*) ( &decodebuffer[0] ), decodebuffer.capacity( ) );
        readsome = true;
      }
      catch ( zstr::Exception& x ) {
        Log::debug( ) << x.what( ) << std::endl;

        // we've gotten some sort of zlib error, so our plan is
        // to skip ahead to the next compressed block, and see
        // if we have better luck. But basically, we'll never
        // classify a zlib error as an application error.

        // try to find the next compressed block, by searching for the
        // next set of zlib magic bytes

        auto skipstart = filestream.tellg( );
        char c;
        bool found = false;
        while ( !found && filestream.get( c ) ) {
          // zlib magic bytes are:
          // 78 01 - No Compression/low
          // 78 5E - Custom Compression
          // 78 9C - Default Compression
          // 78 DA - Best Compression
          if ( 0x78 == c ) {
            short nextc = filestream.peek( );
            found = ( 0x01 == nextc || 0x9C == nextc || 0xDA == nextc || 0x5E == nextc );
          }
        }

        Log::warn( ) << "damaged segment detected...";
        if ( found ) {
          // we found another segment, so rewind to get the 0x78 byte back on the stream
          filestream.seekg( -1, std::ios_base::cur );
          found = true;

          Log::warn( ) << "skipping to next segment" << std::endl;
          auto skipped = filestream.tellg( ) - skipstart;
          Log::debug( ) << "skipping to byte: " << filestream.tellg( ) << " (" << skipped << " bytes ahead)" << std::endl;

          delete zipstream;
          zipstream = new zstr::istream( filestream.rdbuf( ) );
        }
        else {
          Log::warn( ) << "no additional segments found." << std::endl;
          return 0;
        }
      }
    }

    std::streamsize cnt = zipstream->gcount( );
    for ( int i = 0; i < cnt; i++ ) {
      work.push( decodebuffer[i] );
    }

    return cnt;
  }
}