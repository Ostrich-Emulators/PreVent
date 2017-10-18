
#include "Reader.h"
#include "SignalData.h"
#include "WfdbReader.h"
#include "ZlReader.h"
#include "Hdf5Reader.h"
#include "StpXmlReader.h"
#include "CpcXmlReader.h"

#include <iostream>

const std::string Reader::MISSING_VALUESTR( "-32768" );

Reader::Reader( const std::string& name ) : largefile( false ), rdrname( name ),
quiet( false ), anon( false ) {
}

Reader::Reader( const Reader& ) : rdrname( "x" ), quiet( false ), anon( false ) {

}

Reader::~Reader( ) {
}

std::string Reader::name( ) const {
  return rdrname;
}

std::unique_ptr<Reader> Reader::get( const Format& fmt ) {
  switch ( fmt ) {
    case WFDB:
      return std::unique_ptr<Reader>( new WfdbReader( ) );
    case DSZL:
      return std::unique_ptr<Reader>( new ZlReader( ) );
    case STPXML:
      return std::unique_ptr<Reader>( new StpXmlReader( ) );
    case HDF5:
      return std::unique_ptr<Reader>( new Hdf5Reader( ) );
    case CPCXML:
      return std::unique_ptr<Reader>( new CpcXmlReader( ) );
    default:
      throw "reader not yet implemented";
  }
}

int Reader::prepare( const std::string& input, SignalSet& info ) {
  info.reset( false );

  if ( "-" == input || "-zl" == input ) {
    largefile = false;
  }
  else {
    // arbitrary: "large file" is anything over 750M
    size_t sz = getSize( input );
    if ( 0 == sz ) {
      return -1;
    }

    largefile = ( sz > 1024 * 1024 * 750 );
  }

  info.setFileSupport( largefile );
  info.addMeta( "Source Reader", name( ) );

  return 0;
}

void Reader::finish( ) {
  ss.clear( );
}

void Reader::extractOnly( const std::string& toExtract ) {
  toextract = toExtract;
}

bool Reader::shouldExtract( const std::string& q ) const {
  return ( toextract.empty( ) ? true : toextract == q );
}

void Reader::setQuiet( bool q ) {
  quiet = q;
}

void Reader::setAnonymous( bool a ) {
  anon = a;
}

bool Reader::anonymizing( ) const {
  return anon;
}

std::ostream& Reader::output( ) const {
  return ( quiet ? ( std::ostream& ) ss : std::cout );
}