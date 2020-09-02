/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "CsvReader.h"
#include "SignalUtils.h"
#include "Log.h"
#include "SignalData.h"

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

    for ( auto& x : headings ) {
      auto signal = data->addVital( x );
      signal->setChunkIntervalAndSampleRate( 1, 1 );
    }

    auto line = std::string{ };
    while ( datafile.good( ) ) {
      std::getline( datafile, line );
      SignalUtils::trim( line );
      if ( !line.empty( ) ) {
        dr_time rowtime;
        auto vals = linevalues( line, rowtime );

        for ( int i = 1; i < vals.size( ); i++ ) {
          if ( !vals[i].empty( ) ) {
            auto signal = data->addVital( headings[i] );
            signal->add( DataRow::one( rowtime, vals[i] ) );
          }
        }
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

  std::vector<std::string> CsvReader::linevalues( const std::string& csvline, dr_time& timer ) {
    auto strings = SignalUtils::splitcsv( csvline );
    strings.resize( headings.size( ), "" );

    auto timestr = strings[0];
    timer = converttime( timestr );

    return strings;
  }
}