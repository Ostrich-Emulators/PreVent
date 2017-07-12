
#include "Reader.h"
#include "SignalData.h"
#include "WfdbReader.h"
#include "ZlReader.h"
#include "StpXmlReader.h"

Reader::Reader( ) : largefile( false ) {
}

Reader::Reader( const Reader& ) {

}

Reader::~Reader( ) {
}

std::unique_ptr<Reader> Reader::get( const Format& fmt ) {
  switch ( fmt ) {
    case WFDB:
      return std::unique_ptr<Reader>( new WfdbReader( ) );
    case DSZL:
      return std::unique_ptr<Reader>( new ZlReader( ) );
    case STPXML:
      return std::unique_ptr<Reader>( new StpXmlReader( ) );
  }
}

int Reader::reset( const std::string& input, ReadInfo& info ) {
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

ReadResult Reader::fill( ReadInfo& read ) {
  return readChunk( read );
}

int Reader::prepare( const std::string& input, ReadInfo& ) {
  return 0;
}

void Reader::finish( ) {

}