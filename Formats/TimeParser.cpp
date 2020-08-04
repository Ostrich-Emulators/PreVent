/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TimeParser.h"
#include <iomanip>
#include <ctime>
#include <iostream>
#include "Reader.h"
#include "Log.h"

namespace FormatConverter{

  dr_time TimeParser::parse( const std::string& timestr ) {
    if ( timestr.empty( ) ) {
      return 0;
    }

    // timestamp
    if ( timestr.find_first_not_of( "0123456789" ) == std::string::npos ) {
      return std::stol( timestr );
    }

    // check a variety of formats
    const std::string formats[] = {
      "%Y-%m-%dT%T", // 2018-01-31T12:14:59
      "%Y-%m-%d",
      "%m/%d/%Y %I:%M:%S %p",
      "%m/%d/%YT%H:%M:%S", // 6/15/1975T11:45
      "%m/%d/%YT%H:%M", // 6/15/1975T11:45
      "%m/%d/%Y", // 6/15/1975
      "%m/%d/%y", // 6/15/75
    };

    tm tm = { };
    for ( auto& fmt : formats ) {
      // not sure what this will do on failure
      if ( Reader::strptime2( timestr.c_str( ), fmt.c_str( ), &tm ) ) {
        // now convert our local time to UTC
        time_t gmt = timegm( &tm );
        return gmt * 1000; // convert seconds to ms
      }
    }

    Log::error( ) << "could not parse time: " << timestr << std::endl;
    return 0;
  }
}