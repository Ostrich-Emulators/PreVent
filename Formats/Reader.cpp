
#include "Reader.h"
#include "SignalData.h"
#include "WfdbReader.h"
#include "ZlReader2.h"
#include "Hdf5Reader.h"
#include "StpXmlReader.h"
#include "StpJsonReader.h"
#include "CpcXmlReader.h"
#include "TdmsReader.h"

#include <iostream>       // std::cout, std::ios
#include <sstream>        // std::istringstream
#include <ctime>          // std::tm
#include <locale>         // std::locale, std::time_get, std::use_facet
#include <iomanip>

Reader::Reader( const std::string& name ) : largefile( false ), rdrname( name ),
quiet( false ), onefile( false ), local_time( false ) {
}

Reader::Reader( const Reader& r ) : rdrname( "x" ), quiet( r.quiet ),
onefile( r.onefile ), local_time( r.local_time ) {
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
      return std::unique_ptr<Reader>( new ZlReader2( ) );
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

int Reader::prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) {
  std::string timezone = ( 0 == info->metadata( ).count( SignalData::TIMEZONE )
      ? "UTC"
      : info->metadata( ).at( SignalData::TIMEZONE ) );
  info->reset( false );
  info->setMeta( SignalData::TIMEZONE, timezone );
  info->setMeta( "Source Reader", name( ) );
  return 0;
}

void Reader::finish( ) {
  ss.clear( );
}

void Reader::setQuiet( bool q ) {
  quiet = q;
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

bool Reader::getAttributes( const std::string& inputfile, std::map<std::string, std::string>& map ) {
  return false;
}

void Reader::strptime2( const std::string& input, const std::string& format,
    std::tm * tm ) {
  std::istringstream iss( input );
  iss >> std::get_time( tm, format.c_str() );
}
