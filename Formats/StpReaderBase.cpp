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
#include <cstdlib>
#include <memory>

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

    // keep in mind that STP files are a concatenation of zlib-compressed segments,
    // so you're not necessarily done when the zlib is inflated. there could be
    // another segment after it

    if ( nullptr != getenv( "STPGE_INFLATE" ) ) {
      auto path = std::filesystem::path{ filename };
      path.replace_extension( "segs" );
      inflate( filename, path );
    }

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
          Log::warn( ) << "no additional segments found" << std::endl;
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

  void StpReaderBase::inflate( const std::string& input, const std::string& output ) {
    if ( input == output ) {
      Log::error( ) << "not inflating file into itself!" << std::endl;
      return;
    }

    Log::info( ) << "inflating " << input << " to " << output << std::endl;
    std::ifstream fs;
    fs.open( input, std::ifstream::in | std::ifstream::binary );
    auto zippy = new zstr::istream( fs.rdbuf( ) );

    std::ofstream outy( output );

    std::vector<char> decodebuffer;
    decodebuffer.reserve( 1024 * 1024 * 4 );

    // basically the same logic as readMore, but all at once and straight to a file
    while ( !fs.eof( ) ) {
      try {
        zippy->read( decodebuffer.data( ), decodebuffer.capacity( ) );
        auto g = zippy->gcount( );
        if ( g > 0 ) {
          outy.write( decodebuffer.data( ), g );
          outy.flush( );
        }
        else {
          break;
        }
      }
      catch ( zstr::Exception& x ) {
        auto skipstart = fs.tellg( );
        char c;
        bool found = false;
        while ( !found && fs.get( c ) ) {
          if ( 0x78 == c ) {
            short nextc = fs.peek( );
            //Log::debug( ) << "found 0x78 at " << std::dec << fs.tellg( ) << " => 0x" << std::hex << std::setw( 2 ) << std::setfill( '0' ) << nextc << std::endl;
            found = ( 0x01 == nextc || 0x9C == nextc || 0xDA == nextc || 0x5E == nextc );
            //Log::debug( ) << "found new compressed segment: " << found << std::endl;
          }
        }

        Log::debug( ) << x.what( ) << std::endl;
        Log::warn( ) << "damaged segment detected around byte " << std::dec << skipstart << "...";
        if ( found ) {
          // we found another segment, so rewind to get the 0x78 byte back on the stream
          fs.seekg( -1, std::ios_base::cur );
          found = true;

          Log::warn( ) << "skipping to next segment" << std::endl;
          auto skipped = fs.tellg( ) - skipstart;
          Log::debug( ) << "skipping to byte: " << fs.tellg( ) << " (" << skipped << " bytes ahead)" << std::endl;

          delete zippy;
          zippy = new zstr::istream( fs.rdbuf( ) );
        }
        else {
          Log::warn( ) << "no additional segments found" << std::endl;
        }
      }
    }

    outy.close( );
    filestream.close( );
    delete zippy;
    Log::info( ) << "inflated" << std::endl;
  }
}