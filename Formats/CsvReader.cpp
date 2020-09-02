/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "CsvReader.h"
#include "SignalUtils.h"
#include "Log.h"

namespace FormatConverter{

  CsvReader::CsvReader( ) : Reader( "CSV" ) { }

  CsvReader::~CsvReader( ) { }

  int CsvReader::prepare( const std::string& input, SignalSet * info ) {
    datafile.open( input );
    if ( !datafile.good( ) ) {
      Log::error( ) << "no CSV file found" << std::endl;
      return -1;
    }

    std::string firstline;
    std::getline( datafile, firstline );
    headings = SignalUtils::splitcsv( firstline );
    return 0;
  }

  ReadResult CsvReader::fill( SignalSet * data, const ReadResult& lastfill ) {

    auto line = std::string{ };
    while ( datafile.good( ) ) {
      std::getline( datafile, line );
      dr_time rowtime;
      auto map = linevalues( line, rowtime );
      for ( auto& x : map ) {
        Log::error( ) << rowtime << "\t" << x.first << ": " << x.second << std::endl;
      }

    }

    return ReadResult::END_OF_FILE;
  }

  dr_time CsvReader::converttime( const std::string& timer ) {

    if ( std::string::npos == timer.find( " " )
        && std::string::npos == timer.find( "T" ) ) {
      // no space and no T---we must have a unix timestamp
      // and we believe Stp saves unix timestamps in UTC always
      // (regardless of valIsLocal)
      return modtime( std::stol( timer ) * 1000 );
    }

    // we have a local time that we need to convert
    std::string format = ( std::string::npos == timer.find( "T" )
        ? "%m/%d/%Y %I:%M:%S %p" // STPXML time string
        : "%Y-%m-%dT%H:%M:%S" ); // CPC time string

    tm mytime = { 0, };

    strptime2( timer, format, &mytime );
    //output( ) << mytime.tm_hour << ":" << mytime.tm_min << ":" << mytime.tm_sec << std::endl;

    time_t local = mktime( &mytime );
    mytime = *gmtime( &local );
    //output( ) << mytime.tm_hour << ":" << mytime.tm_min << ":" << mytime.tm_sec << std::endl;

    return modtime( mktime( &mytime )* 1000 ); // convert seconds to ms, adds offset
  }

  std::map<std::string, std::string> CsvReader::linevalues( const std::string& csvline, dr_time& timer ) {
    std::vector<std::string> strings = SignalUtils::splitcsv( csvline );
    std::map<std::string, std::string> values;

    std::string timestr = strings[0];
    timer = converttime( timestr );
    for ( size_t i = 1; i < headings.size( ); i++ ) {
      const auto& h = headings[i];
      const auto& v = SignalUtils::trim( strings[i] );
      if ( v.size( ) > 0 ) {
        values[h] = v;
      }
    }
    return values;
  }
}