
#include "FromReader.h"
#include "SignalData.h"
#include "WfdbReader.h"
#include "ZlReader.h"

FromReader::FromReader( ) : largefile( false ) {
}

FromReader::FromReader( const FromReader& ) {

}

FromReader::~FromReader( ) {
}

std::unique_ptr<FromReader> FromReader::get( const Format& fmt ) {
  switch ( fmt ) {
    case WFDB:
      return std::unique_ptr<FromReader>( new WfdbReader( ) );
    case DSZL:
      return std::unique_ptr<FromReader>( new ZlReader() );
  }
}

int FromReader::reset( const std::string& input, ReadInfo& info ) {
  info.reset( false );

  if ( "-" == input || "-zl" == input ) {
    largefile = false;
  }
  else {
    // arbitrary: "large file" is anything over 750M
    int sz = getSize( input );
    if ( sz < 0 ) {
      return -1;
    }

    largefile = ( sz > 1024 * 1024 * 750 );
  }

  info.setFileSupport( largefile );
  prepare( input, info );
  
  return 0;
}

ReadResult FromReader::fill( ReadInfo& read ) {
  return readChunk( read );
}

int FromReader::prepare( const std::string& input, ReadInfo& ){
  return 0;
}

void FromReader::finish(){

}