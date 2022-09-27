/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TimeParser.h
 * Author: ryan
 *
 * Created on March 26, 2018, 11:54 AM
 */

#ifndef TIMEPARSER_H
#define TIMEPARSER_H

#include "dr_time.h"
#include <string>

namespace FormatConverter {

  class TimeParser {
  public:

    virtual ~TimeParser( ) { }

    /**
     * Parses time and returns the seconds since the epoch.
     * @param timestr the string to parse. could be a time string, or seconds
     * since the epoch.
     * @param islocal if true, assume the timestr is localtime. (This does nothing
     * if timestr is actually a number of seconds since the epoch.)
     * @return
     */
    static time_t parse( const std::string& timestr, bool islocal = false );

    static std::string format( dr_time time, const std::string& fmt = "%Y/%m/%d %H:%M:%S",
        bool islocal = false );

  private:

    TimeParser( );

    TimeParser( const TimeParser& ) { }
  };
}
#endif /* TIMEPARSER_H */

