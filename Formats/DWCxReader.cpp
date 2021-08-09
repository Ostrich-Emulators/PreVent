/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "DWCxReader.h"
#include "SignalUtils.h"
#include "Log.h"
#include "SignalData.h"

#include <filesystem>

namespace FormatConverter{

  DWCxReader::DWCxReader( ) : Reader( "DWCxCSV" ) { }

  DWCxReader::~DWCxReader( ) { }

  int DWCxReader::prepare( const std::string& input, SignalSet * info ) {
    datafile.open( input );
    if ( !datafile.good( ) ) {
      Log::error( ) << "no CSV file found" << std::endl;
      return -1;
    }

    auto metainput = std::filesystem::path{ input };
    metainput = metainput.remove_filename( );
    metainput += "DWC_Parameter-Alerts_Plus_3rdParty_JI.csv";

    auto metadatafile = std::fstream{ metainput };
    if ( metadatafile.good( ) ) {
      Log::debug( ) << "loading labels from file: " << metainput << std::endl;
      auto line = std::string{ };
      while ( std::getline( metadatafile, line ) ) {
        if ( !SignalUtils::trim( line ).empty( ) ) {
          auto vec = SignalUtils::splitcsv( line );
          auto code = vec[0];
          auto label = vec[1];
          auto subcode = ( vec.size( ) > 2
              ? vec[2]
              : "" );

          if ( subcode.empty( ) ) {
            subcode = code;
          }

          auto key = code + subcode;
          codeslabellkp.insert( std::pair<std::string, std::string>( key, label ) );
        }
      }
    }
    else {
      Log::trace( ) << "no meta file: " << metainput << std::endl;
    }

    return 0;
  }

  ReadResult DWCxReader::fill( SignalSet * data, const ReadResult& lastfill ) {
    //loadMetas( data );

    auto line = std::string{ };
    while ( datafile.good( ) ) {
      auto pos = datafile.tellg( );
      std::getline( datafile, line );
      SignalUtils::trim( line );
      if ( !line.empty( ) ) {
        dr_time rowtime;
        auto vals = linevalues( line, rowtime );

        if ( isRollover( rowtime, data ) ) {
          datafile.seekg( pos );
          return ReadResult::END_OF_DURATION;
        }

        data->setMeta( "Patient MRN", vals[0] );

        auto code = vals[1];
        auto subcode = vals[2];
        auto key = code + subcode;

        //Log::error( ) << code << "," << subcode << std::endl;
        if ( codeslabellkp.find( key ) == codeslabellkp.end( ) ) {
          if ( codeslabellkp.find( subcode + subcode ) != codeslabellkp.end( ) ) {
            if ( warnings.find( key ) == warnings.end( ) ) {
              warnings.insert( key );
              Log::warn( ) << "code/subcode not found: " << code << "," << subcode
                  << "; using:" << subcode << "," << subcode << std::endl;
            }
            key = subcode + subcode;
          }
          else {
            Log::error( ) << "code/subcode not found: " << code << "," << subcode << std::endl;
            return ReadResult::ERROR;
          }
        }

        auto signal = data->addVital( codeslabellkp.at( key ) );
        signal->setChunkIntervalAndSampleRate(1000, 1);
        signal->add( DataRow::one( rowtime, vals[3] ) );
      }
    }

    return ReadResult::END_OF_FILE;
  }

  dr_time DWCxReader::converttime( const std::string& timer ) {
    // we have a local time that we need to convert
    std::string format = "%Y-%m-%d %H:%M:%S";

    tm mytime = { 0, };

    strptime2( timer, format, &mytime );
    //output( ) << mytime.tm_hour << ":" << mytime.tm_min << ":" << mytime.tm_sec << std::endl;

    time_t local = mktime( &mytime );
    mytime = *gmtime( &local );
    //output( ) << mytime.tm_hour << ":" << mytime.tm_min << ":" << mytime.tm_sec << std::endl;

    return modtime( mktime( &mytime )* 1000 ); // convert seconds to ms, adds offset
  }

  std::vector<std::string> DWCxReader::linevalues( const std::string& csvline, dr_time& timer ) {
    auto strings = SignalUtils::splitcsv( csvline );
    //strings.resize( headings.size( ), "" );

    auto timestr = strings[4];
    timer = converttime( timestr );

    return strings;
  }
}