/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "CsvReader.h"
#include "SignalUtils.h"
#include "Log.h"
#include "SignalData.h"

#include <filesystem>

namespace FormatConverter{

  CsvReader::CsvReader( const std::string& name, bool firstHeader, int timef, int fc )
      : Reader( "CSV" ), firstLineIsHeader( firstHeader ), timefield( timef ), fieldcount( fc ) { }

  CsvReader::~CsvReader( ) { }

  int CsvReader::prepare( const std::string& input, SignalSet * info ) {
    datafile.open( input );
    if ( !datafile.good( ) ) {
      Log::error( ) << "no CSV file found" << std::endl;
      return -1;
    }

    auto metainput = std::filesystem::path{ input };
    metainput.replace_extension( ".meta" );
    auto metadatafile = std::fstream{ metainput };
    if ( metadatafile.good( ) ) {
      Log::debug( ) << "loading metadata from file: " << metainput << std::endl;
      auto line = std::string{ };
      while ( std::getline( metadatafile, line ) ) {
        if ( !SignalUtils::trim( line ).empty( ) ) {
          metadata.push_back( line );
        }
      }
    }
    else {
      Log::trace( ) << "no meta file: " << metainput << std::endl;
    }

    if ( this->firstLineIsHeader ) {
      std::string firstline;
      std::getline( datafile, firstline );
      headings = SignalUtils::splitcsv( firstline );
      fieldcount = headings.size( );
    }

    if ( timefield > fieldcount - 1 ) {
      timefield = 0;
    }

    return 0;
  }

  ReadResult CsvReader::fill( SignalSet * data, const ReadResult& lastfill ) {
    loadMetas( data );

    auto line = std::string{ };
    auto firstline = true;
    while ( datafile.good( ) ) {
      auto pos = datafile.tellg( );
      std::getline( datafile, line );
      SignalUtils::trim( line );
      if ( !line.empty( ) ) {
        dr_time rowtime;
        auto vals = linevalues( line, rowtime );

        if ( firstline ) {
          firstline = false;
          this->setMetas( vals, data );
        }

        if ( isRollover( rowtime, data ) ) {
          datafile.seekg( pos );
          return ReadResult::END_OF_DURATION;
        }
        else if ( isNewPatient( vals, data ) ) {
          datafile.seekg( pos );
          return ReadResult::END_OF_PATIENT;
        }

        for ( size_t i = 0; i < vals.size( ); i++ ) {
          if ( this->includeFieldValue( i, vals ) ) {
            auto first = true;
            auto signal = data->addVital( this->headerForField( i, vals ), &first );
            if ( first ) {
              signal->setChunkIntervalAndSampleRate( 1, 1 );
            }
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
      // no space and no T---we must have a timestamp
      // if the string length is too long, assume we have a ms timestamp

      const auto scale = ( timer.size( ) > 10
          ? 1
          : 1000 );

      return modtime( std::stol( timer ) * scale );
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

    strings.resize( fieldcount, "" );

    auto timestr = strings[timefield];
    timer = converttime( timestr );

    return strings;
  }

  std::string CsvReader::headerForField( int field, const std::vector<std::string>& linevals ) const {
    return headings[field];
  }

  bool CsvReader::includeFieldValue( int field, const std::vector<std::string>& vals ) const {
    return !( headings.empty( ) || isTimeField( field ) || vals[field].empty( ) );
  }

  bool CsvReader::isNewPatient( const std::vector<std::string>& linevals, SignalSet * info ) const {
    return false;
  }

  bool CsvReader::isTimeField( int field ) const {
    return field == timefield;
  }

  void CsvReader::setMetas( const std::vector<std::string>& linevals, SignalSet * data ) { }

  void CsvReader::loadMetas( SignalSet * info ) {

    // root metadata starts with /|
    for ( auto&line : metadata ) {
      auto check = "/|";
      if ( 0 == line.find( check ) ) {
        auto pieces = SignalUtils::splitcsv( line, '|' );
        info->setMeta( pieces[1], pieces[2] );
      }
    }

    // now set data for individual vitals
    for ( size_t i = 1; i < headings.size( ); i++ ) {
      auto signal = info->addVital( headings[i] );
      signal->setChunkIntervalAndSampleRate( 1, 1 );
      for ( auto&line : metadata ) {
        auto check = "/VitalSigns/" + headings[i] + "|";
        if ( 0 == line.find( check ) ) {
          auto pieces = SignalUtils::splitcsv( line, '|' );
          signal->setMeta( pieces[1], pieces[2] );
        }
      }
    }
  }
}