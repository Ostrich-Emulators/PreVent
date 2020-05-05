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

#include <sys/stat.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

namespace FormatConverter{

  StpReaderBase::StpReaderBase( const std::string& name ) : Reader( name ),
      work( 1024 * 1024 ), metadataonly( false ) { }

  StpReaderBase::StpReaderBase( const StpReaderBase& orig ) : Reader( orig ),
      work( orig.work.capacity( ) ), metadataonly( orig.metadataonly ) { }

  StpReaderBase::~StpReaderBase( ) { }

  void StpReaderBase::setMetadataOnly( bool metasonly ) {
    metadataonly = metasonly;
  }

  bool StpReaderBase::isMetadataOnly( ) const {
    return metadataonly;
  }

  int StpReaderBase::prepare( const std::string& filename, std::unique_ptr<SignalSet>& data ) {
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
    //    std::ofstream outy( "uncompressed.segs" );
    //    do {
    //      doublereader.read( skipper, 1024 * 1024 );
    //      for ( size_t i = 0; i < 1024 * 1024; i++ ) {
    //        outy << skipper[i];
    //      }
    //    }
    //    while ( doublereader.gcount( ) > 0 );
    //    outy.close( );
    //    delete []skipper;

    filestream = new zstr::ifstream( filename );
    return 0;
  }

  void StpReaderBase::finish( ) {
    if ( filestream ) {
      delete filestream;
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

  int StpReaderBase::readMore( ) {
    std::vector<unsigned char> decodebuffer;
    decodebuffer.reserve( 1024 * 64 );
    try {
      filestream->read( (char*) ( &decodebuffer[0] ), decodebuffer.capacity( ) );
    }
    catch ( zstr::Exception& x ) {
      std::cerr << x.what( ) << std::endl;
      return -1;
    }

    std::streamsize cnt = filestream->gcount( );
    for ( int i = 0; i < cnt; i++ ) {
      work.push( decodebuffer[i] );
    }

    return cnt;
  }
}
