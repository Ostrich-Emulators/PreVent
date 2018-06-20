
#include "Reader.h"
#include "SignalData.h"
#include "WfdbReader.h"
#include "ZlReader.h"
#include "Hdf5Reader.h"
#include "StpXmlReader.h"
#include "StpJsonReader.h"
#include "CpcXmlReader.h"
#include "TdmsReader.h"

#include <iostream>

Reader::Reader( const std::string& name ) : largefile( false ), rdrname( name ),
quiet( false ), anon( false ), onefile( false ), local_time( false ) {
  // figure out a string for our timezone by getting a reference time
  time_t reftime = std::time( nullptr );
  tm * reftm = localtime( &reftime );
  gmt_offset = reftm->tm_gmtoff;
  timezone = std::string( reftm->tm_zone );
}

Reader::Reader( const Reader& r ) : rdrname( "x" ), quiet( r.quiet ), anon( r.anon ),
onefile( r.onefile ), local_time( r.local_time ), gmt_offset( r.gmt_offset ),
timezone( r.timezone ) {
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
    case STPJSON:
      return std::unique_ptr<Reader>( new StpJsonReader( ) );
    case TDMS:
      return std::unique_ptr<Reader>( new TdmsReader( ) );
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
  largefile=true;
  info.setFileSupport( largefile );
  info.addMeta( "Source Reader", name( ) );

  // figure out a string for our timezone by getting a reference time
  info.addMeta( SignalData::TIMEZONE, localizingTime( ) ? tz_name() : "UTC" );

  return 0;
}

int Reader::tz_offset() const{
  return gmt_offset;
}

const std::string& Reader::tz_name() const {
  return timezone;
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

void Reader::setNonbreaking( bool nb ) {
  onefile = nb;
}

bool Reader::nonbreaking( ) const {
  return onefile;
}

void Reader::localizeTime( bool local ) {
  local_time = local;
}

bool Reader::localizingTime( ) const {
  return local_time;
}

std::ostream& Reader::output( ) const {
  return ( quiet ? ( std::ostream& ) ss : std::cout );
}