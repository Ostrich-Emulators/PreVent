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

class TimeParser {
public:

  virtual ~TimeParser( ) {
  }

  static dr_time parse( const std::string& timestr );


private:

  TimeParser( );

  TimeParser( const TimeParser& ) {
  }
};


#endif /* TIMEPARSER_H */

