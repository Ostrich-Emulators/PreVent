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
#include <cassert>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

namespace FormatConverter {

  StpReaderBase::StpReaderBase( const std::string& name ) : Reader( name ),
      work( 1024 * 1024 * 16 ), metadataonly( false ), warnederror( false ) {
  }

  StpReaderBase::StpReaderBase( const StpReaderBase& orig ) : Reader( orig ),
      work( orig.work.capacity( ) ), metadataonly( orig.metadataonly ),
      warnederror( orig.warnederror ) {
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

    /* allocate inflate state */
    zipdata.zalloc = Z_NULL;
    zipdata.zfree = Z_NULL;
    zipdata.opaque = Z_NULL;
    zipdata.avail_in = 0;
    zipdata.next_in = Z_NULL;
    auto ret = inflateInit( &zipdata );
    if ( ret != Z_OK ) {
      Log::error( ) << "could not initialize compression handler" << std::endl;
      return -1;
    }

    if ( nullptr != getenv( "STPGE_INFLATE" ) ) {
      auto path = std::filesystem::path{ filename };
      path.replace_extension( "segs" );
      inflate_f( filename, path );
    }

    filestream.open( filename, std::ifstream::in | std::ifstream::binary );
    return 0;
  }

  void StpReaderBase::finish( ) {
    inflateEnd( &zipdata );

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
    if ( !filestream ) {
      return 0;
    }

    size_t INSIZE = 1024 * 128;
    unsigned char * indata = new unsigned char[INSIZE];

    size_t OUTSIZE = 1024 * 1024 * 8;
    unsigned char * outdata = new unsigned char[OUTSIZE];

    filestream.read( (char *) indata, INSIZE );
    size_t read = filestream.gcount( );

    auto atEOF = filestream.eof();

    if ( read > 0 ) {
      auto ok = inflate_b( indata, read, outdata, OUTSIZE );
      if ( Z_OK == ok ) {
        if ( read > 0 ) {
          // put the extra data back in the stream
          Log::trace( ) << "returning " << read << " bytes back to the stream" << std::endl;
          if ( atEOF ) {
            filestream.clear( );
            filestream.seekg( -read, std::ios_base::end );
          }
          else {
            filestream.seekg( -read, std::ios_base::cur );
          }
        }

        for ( size_t i = 0; i < OUTSIZE; i++ ) {
          work.push( outdata[i] );
        }

        warnederror = false;
      }
      else {
        OUTSIZE = 0;
        if( !warnederror ){
          Log::warn( ) << "compression error: " << zerr( ok ) << std::endl;
          warnederror = true;
        }

        // we always inflate our indata array starting at the beginning
        // in this case, that segment is damaged, so iterate through until
        // we find another segment, and return everything after that to the
        // stream so it can get picked up in a recursion.
        auto found = -1;
        for ( size_t i = 2; i < read - 1 && found < 0; i++ ) {
          if ( 0x78 == indata[i] && ( 0x01 == indata[i + 1] || 0x9C == indata[i + 1] || 0xDA == indata[i + 1] ) ) {
            found = i;
          }
        }

        if ( found > 0 ) {
          auto rewind = ( read - found );
          if ( atEOF ) {
            filestream.clear( );
            filestream.seekg( -rewind, std::ios_base::end );
          }
          else {
            filestream.seekg( -rewind, std::ios_base::cur );
          }

          delete [] indata;
          delete [] outdata;
          return readMore( );
        }
      }
    }

    delete [] indata;
    delete [] outdata;
    return OUTSIZE;
  }

  void StpReaderBase::inflate_f( const std::string& input, const std::string& output ) {
    if ( input == output ) {
      Log::error( ) << "not inflating file into itself!" << std::endl;
      return;
    }

    // this is basically the same logic as readMore, but all at once
    // and straight to a file

    Log::info( ) << "inflating " << input << " to " << output << std::endl;
    std::ofstream outy( output );
    std::ifstream inny;
    inny.open( input, std::ifstream::in | std::ifstream::binary );

    const size_t INSIZE = 1024 * 128;
    unsigned char * indata = new unsigned char[INSIZE];

    const size_t OUTSIZE = 1024 * 1024 * 8;
    unsigned char * outdata = new unsigned char[OUTSIZE];

    while ( inny ) {
      auto insize = INSIZE;
      auto fileg = inny.tellg( );
      inny.read( (char *) indata, insize );
      size_t read = inny.gcount( );

      auto atEOF = inny.eof();

      Log::debug( ) << "read " << read << " of " << INSIZE << " bytes from file byte:" << fileg << std::endl;
      if ( read > 0 ) {
        auto outsize = OUTSIZE;
        auto ok = inflate_b( indata, read, outdata, outsize );
        if ( Z_OK == ok ) {
          if ( read > 0 ) {
            // put the extra data back in the stream
            Log::trace( ) << "returning " << read << " bytes back to the stream" << std::endl;
            if ( atEOF ) {
              inny.clear( );
              inny.seekg( -read, std::ios_base::end );
            }
            else {
              inny.seekg( -read, std::ios_base::cur );
            }
          }

          outy.write( (char *) outdata, outsize );
        }
        else {
          Log::warn( ) << "problem inflating data: " << zerr( ok ) << std::endl;

          // we always try to inflate our indata array starting at the beginning
          // in this case, that segment is damaged, so iterate through until
          // we find another segment, and return everything after that to the
          // stream so it can get picked up on the next loop.
          auto found = -1;
          for ( size_t i = 2; i < read - 1 && found < 0; i++ ) {
            if ( 0x78 == indata[i] && ( 0x01 == indata[i + 1] || 0x9C == indata[i + 1] || 0xDA == indata[i + 1] ) ) {
              found = i;
            }
          }

          if ( found > 0 ) {
            auto rewind = ( read - found );
            if ( atEOF ) {
              inny.clear( );
              inny.seekg( -rewind, std::ios_base::end );
            }
            else {
              inny.seekg( -rewind, std::ios_base::cur );
            }
          }
        }
      }
    }

    outy.close( );
    inny.close( );
    delete [] indata;
    delete [] outdata;
    Log::info( ) << "inflated" << std::endl;
  }

  int StpReaderBase::inflate_b( unsigned char * input, size_t& insize,
      unsigned char * output, size_t& outsize ) {
    // slightly modified from http://zlib.net/zpipe.c
    zipdata.avail_in = insize;
    if ( zipdata.avail_in == 0 ) {
      return Z_STREAM_END;
    }
    zipdata.next_in = input;

    inflateReset( &zipdata );

    int ret;

    /* decompress until deflate stream ends */
    // we expect the output array to be big enough to hold an entire
    // compressed segment
    do {
      // we expect to inflate all the data from input in one pass
      zipdata.avail_out = outsize;
      zipdata.next_out = output;
      Log::debug( ) << "inflating..." << std::endl;
      ret = inflate( &zipdata, Z_NO_FLUSH );
      assert( ret != Z_STREAM_ERROR ); /* state not clobbered */
      switch ( ret ) {
        case Z_NEED_DICT:
          ret = Z_DATA_ERROR; /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          return ret;
      }

      Log::trace( ) << "inflated " << ( insize - zipdata.avail_in ) << " bytes to "
          << ( outsize - zipdata.avail_out ) << " bytes with " << zipdata.avail_in << " left over. " << zerr( ret ) << std::endl;
      /* done when inflate() says it's done */

      insize = zipdata.avail_in;
      outsize = ( outsize - zipdata.avail_out );

      if ( 0 == insize ) {
        // nothing left to convert, so say we're at the end (nervously)
        ret = Z_STREAM_END;
      }
    }
    while ( ret != Z_STREAM_END );

    /* clean up and return */
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
  }

  std::string StpReaderBase::zerr( int Z ) {
    switch ( Z ) {
      case Z_OK:
        return "no error (ok)";
      case Z_STREAM_END:
        return "no error (end)";
      case Z_ERRNO:
        return "error reading input";
      case Z_STREAM_ERROR:
        return "invalid compression level";
      case Z_DATA_ERROR:
        return "invalid or incomplete deflate data";
      case Z_MEM_ERROR:
        return "out of memory";
      case Z_VERSION_ERROR:
        return "zlib version mismatch";
      default:
        return "unknown error: " + Z;
    }
  }
}