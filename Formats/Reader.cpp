
#include "Reader.h"
#include "SignalData.h"
#include "WfdbReader.h"
#include "ZlReader2.h"
#include "Hdf5Reader.h"
#include "StpXmlReader.h"
#include "StpJsonReader.h"
#include "CpcXmlReader.h"
#include "CsvReader.h"
#include "TdmsReader.h"
#include "StpGeReader.h"
#include "StpPhilipsReader.h"
#include "DwcReader.h"
#include "Options.h"
#include "Log.h"

#include <iostream>       // std::cout, std::ios
#include <sstream>        // std::istringstream
#include <ctime>          // std::tm
#include <locale>         // std::locale, std::time_get, std::use_facet
#include <iomanip>

namespace FormatConverter{

  Reader::Reader( const std::string& name ) : rdrname( name ), onefile( false ),
      local_time( false ), timemod( TimeModifier::passthru( ) ) { }

  Reader::Reader( const Reader& r ) : rdrname( "x" ), onefile( r.onefile ),
      local_time( r.local_time ), timemod( r.timemod ) { }

  Reader::~Reader( ) { }

  std::string Reader::name( ) const {
    return rdrname;
  }

  std::unique_ptr<Reader> Reader::get( const Format& fmt ) {
    switch ( fmt ) {
      case FormatConverter::WFDB:
        return std::make_unique<WfdbReader>( );
      case FormatConverter::DSZL:
        return std::make_unique<ZlReader2>( );
      case FormatConverter::STPGE:
        return std::make_unique<StpGeReader>( );
      case FormatConverter::STPP:
        return std::make_unique<StpPhilipsReader>( );
      case FormatConverter::STPXML:
        return std::make_unique<StpXmlReader>( );
      case FormatConverter::HDF5:
        return std::make_unique<Hdf5Reader>( );
      case FormatConverter::CPCXML:
        return std::make_unique<CpcXmlReader>( );
      case FormatConverter::STPJSON:
        return std::make_unique<StpJsonReader>( );
      case FormatConverter::MEDI:
        return std::make_unique<TdmsReader>( );
      case FormatConverter::DWC:
        return std::make_unique<DwcReader>( );
      case FormatConverter::CSV:
        return std::make_unique<CsvReader>( );
      default:
        throw std::runtime_error( "reader not yet implemented" );
    }
  }

  int Reader::prepare( const std::string& input, SignalSet * info ) {
    Log::debug( ) << "preparing to read " << input << std::endl;
    std::string timezone = ( 0 == info->metadata( ).count( SignalData::TIMEZONE )
        ? "UTC"
        : info->metadata( ).at( SignalData::TIMEZONE ) );
    info->reset( false );
    info->setMeta( SignalData::TIMEZONE, timezone );
    info->setMeta( "Source Reader", name( ) );
    return 0;
  }

  void Reader::finish( ) { }

  void Reader::setNonbreaking( bool nb ) {
    onefile = nb;
  }

  bool Reader::nonbreaking( ) const {
    return onefile;
  }

  bool Reader::skipwaves( ) const {
    return Options::asBool( OptionsKey::SKIP_WAVES );
  }

  void Reader::localizeTime( bool local ) {
    local_time = local;
  }

  bool Reader::localizingTime( ) const {
    return local_time;
  }

  bool Reader::getAttributes( const std::string& inputfile, std::map<std::string, std::string>& map ) {
    return false;
  }

  bool Reader::getAttributes( const std::string& inputfile, const std::string& signal,
      std::map<std::string, int>& mapi, std::map<std::string, double>& mapd, std::map<std::string, std::string>& maps,
      dr_time& starttime, dr_time& endtime ) {
    return false;
  }

  bool Reader::isRollover( const dr_time& then, const dr_time& now ) const {
    if ( nonbreaking( ) ) {
      return false;
    }

    if ( 0 != then ) {
      time_t modnow = now / 1000;
      time_t modthen = then / 1000;

      const int cdoy = ( localizingTime( ) ? localtime( &modnow ) : gmtime( &modnow ) )->tm_yday;
      const int pdoy = ( localizingTime( ) ? localtime( &modthen ) : gmtime( &modthen ) )->tm_yday;
      if ( cdoy != pdoy ) {
        return true;
      }
    }

    return false;
  }

  bool Reader::splice( const std::string& inputfile, const std::string& path,
      dr_time from, dr_time to, SignalData * signal ) {
    Log::error( ) << "this reader does not support splicing" << std::endl;
    return false;
  }

  bool Reader::strptime2( const std::string& input, const std::string& format,
      std::tm * tm ) {
    std::istringstream iss( input );
    iss >> std::get_time( tm, format.c_str( ) );
    if ( iss.fail( ) ) {
      return false;
    }
    return true;
  }

  void Reader::timeModifier( const TimeModifier& mod ) {
    timemod = mod;
  }

  const TimeModifier& Reader::timeModifier( ) const {
    return timemod;
  }

  dr_time Reader::modtime( const dr_time& time ) {
    return timemod.convert( time );
  }
}