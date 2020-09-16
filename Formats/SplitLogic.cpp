/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SplitLogic.h"

#include "Reader.h"
#include <ctime>

namespace FormatConverter{

  SplitLogic::SplitLogic( int hrs, bool cleen ) : hours( hrs ), clean( cleen ) { }

  SplitLogic::SplitLogic( const SplitLogic& orig )
      : hours( orig.hours ), clean( orig.clean ) { }

  SplitLogic& SplitLogic::operator=(const SplitLogic& orig ) {
    if ( this != &orig ) {
      hours = orig.hours;
      clean = orig.clean;
    }
    return *this;
  }

  SplitLogic::~SplitLogic( ) { }

  SplitLogic SplitLogic::nonbreaking( ) {
    return SplitLogic( 0 );
  }

  SplitLogic SplitLogic::midnight( ) {
    return SplitLogic( -1 );
  }

  SplitLogic SplitLogic::hourly( int numhours, bool clean ) {
    return SplitLogic( numhours, clean );
  }

  bool SplitLogic::isRollover( dr_time then, dr_time now, const Reader * reader ) const {
    if ( 0 == hours || 0 == then ) {
      return false;
    }

    time_t modnow = now / 1000;
    time_t modthen = then / 1000;

    auto nowtm = *( reader->localizingTime( ) ? localtime( &modnow ) : gmtime( &modnow ) );
    auto thentm = *( reader->localizingTime( ) ? localtime( &modthen ) : gmtime( &modthen ) );

    if ( hours < 0 ) {
      // roll at midnight (when the day of the year changes)
      return ( nowtm.tm_yday != thentm.tm_yday );
    }

    return ( clean
        ? thentm.tm_hour + this->hours >= nowtm.tm_hour
        : then + ( this->hours * 60 * 60 * 1000 ) >= now );
  }
}