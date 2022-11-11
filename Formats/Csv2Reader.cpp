/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Csv2Reader.h"
#include "SignalUtils.h"
#include "Log.h"
#include "SignalData.h"

#include <filesystem>

namespace FormatConverter{

  Csv2Reader::Csv2Reader( ) : CsvReader( "CSV2", true, 4, 5 ) { }

  Csv2Reader::~Csv2Reader( ) { }

  std::string Csv2Reader::headerForField( int field, const std::vector<std::string>& linevals ) const {
    return linevals[2];
  }

  bool Csv2Reader::includeFieldValue( int field, const std::vector<std::string>& vals ) const {
    return 3 == field && !vals[field].empty( );
  }

  bool Csv2Reader::isNewPatient( const std::vector<std::string>& linevals, SignalSet * info ) const {
    if ( 0 != info->metadata( ).count( "MRN" ) ) {
      auto datamrn = info->metadata( ).at( "MRN" );

      auto valmrn = linevals.at( 0 );
      valmrn.erase( std::remove( valmrn.begin( ), valmrn.end( ), '\"' ), valmrn.end( ) );

      return datamrn != valmrn;
    }
    return false;
  }

  void Csv2Reader::setMetas( const std::vector<std::string>& linevals, SignalSet * data ) {
    auto valmrn = linevals.at( 0 );
    valmrn.erase( std::remove( valmrn.begin( ), valmrn.end( ), '\"' ), valmrn.end( ) );
    if ( !valmrn.empty( ) ) {
      data->setMeta( "MRN", valmrn );
    }
  }

  dr_time Csv2Reader::converttime( const std::string& timer ) {

    if ( std::string::npos == timer.find( " " )
        && std::string::npos == timer.find( "T" ) ) {
      // no space and no T---we must have a timestamp
      // if the string length is too long, assume we have a ms timestamp

      const auto scale = ( timer.size( ) > 10
          ? 1
          : 1000 );

      return modtime( std::stol( timer ) * scale );
    }

    // we have a local time that we need to convert
    std::string format = "%Y-%m-%d %H:%M:%S";

    // we need to handle timezone and ms data separately (strptime2 doesn't)
    auto mytimestr = timer;
    auto tzidx = timer.rfind( ' ' );
    auto tzmod = 0;
    if ( std::string::npos != tzidx ) {
      mytimestr = timer.substr( 0, tzidx );

      auto tzstr = timer.substr( tzidx + 1 );
      auto tzcolon = tzstr.find( ':' );
      auto tzh = tzstr.substr( 1, tzcolon - 1 );
      auto tzm = tzstr.substr( tzcolon + 1 );

      tzmod = -std::stoi( tzh ) * 60 * 60 - std::stoi( tzm )*60;

      if ( '-' == tzstr[0] ) {
        tzmod = 0 - tzmod;
      }
    }

    auto msmod = 0;
    if ( std::string::npos != mytimestr.find( '.' ) ) {
      auto idx = timer.find( '.' );
      auto msstr = mytimestr.substr( idx + 1, 3 );
      msmod = std::stoi( msstr );

      mytimestr = mytimestr.substr( 0, idx );
    }


    tm mytime = { 0, };

    strptime2( mytimestr, format, &mytime );

    mytime.tm_gmtoff = tzmod;
    time_t time = timegm( &mytime ) + tzmod;
    return modtime( time * 1000 + msmod ); // convert seconds to ms, adds offset
  }
}