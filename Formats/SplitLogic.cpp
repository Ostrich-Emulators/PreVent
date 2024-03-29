/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SplitLogic.h"

#include "SignalSet.h"
#include "Log.h"
#include <iomanip>
#include <ctime>
#include <limits>

namespace FormatConverter{

  SplitLogic::SplitLogic( int hrs, bool cleen )
      : hours( hrs ), clean( cleen ) { }

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

  SplitLogic SplitLogic::nobreaks( ) {
    return SplitLogic( 0 );
  }

  bool SplitLogic::nonbreaking( ) const {
    return ( 0 == hours );
  }

  SplitLogic SplitLogic::midnight( ) {
    return SplitLogic( -1 );
  }

  SplitLogic SplitLogic::hourly( int numhours, bool clean ) {
    return SplitLogic( numhours, clean );
  }

  bool SplitLogic::isRollover( dr_time latest, dr_time now, bool timesAreLocal ) const {
    if ( 0 == hours ) {
      return false;
    }

    if ( std::numeric_limits<dr_time>::min() == latest ) {
      return false;
    }

    const time_t modnow = now / 1000;
    const time_t modlatest = latest / 1000;

    const auto nowtm = *( timesAreLocal ? localtime( &modnow ) : gmtime( &modnow ) );
    const auto thentm = *( timesAreLocal ? localtime( &modlatest ) : gmtime( &modlatest ) );

    if ( hours < 0 ) {
      // roll at midnight (when the day of the year changes)
      return ( nowtm.tm_yday != thentm.tm_yday );
    }

    // rolling every X number of hours
    if ( clean ) {
      if ( nowtm.tm_yday == thentm.tm_yday ) {
        // not yet crossed midnight
        return ( nowtm.tm_hour - thentm.tm_hour ) >= this->hours;
      }
      else {
        // we're into the next day, so add 24 hours to our now hour
        return ( nowtm.tm_hour + 24 - thentm.tm_hour ) >= this->hours;
      }
    }
    return (now - latest ) >= ( this->hours * 60 * 60 * 1000 );
  }

  bool SplitLogic::isRollover( SignalSet * data, dr_time now, bool nowIsLocal ) const {
    if ( 0 == hours ) {
      return false;
    }

    const auto latest = data->latest( );
    // FIXME: check for min, not 0
    if ( std::numeric_limits<dr_time>::min() == latest ) {
      return false;
    }

    const time_t modnow = now / 1000;
    const time_t modlatest = latest / 1000;

    const auto nowtm = *( nowIsLocal ? localtime( &modnow ) : gmtime( &modnow ) );

    if ( hours < 0 ) {
      const auto thentm = *( nowIsLocal ? localtime( &modlatest ) : gmtime( &modlatest ) );
      // roll at midnight (when the day of the year changes)
      return ( nowtm.tm_yday != thentm.tm_yday );
    }

    const auto earliest = data->earliest( );
    const time_t modearly = earliest / 1000;
    if ( clean ) {
      const auto earlytm = *( nowIsLocal ? localtime( &modearly ) : gmtime( &modearly ) );

      if ( nowtm.tm_yday == earlytm.tm_yday ) {
        // not yet crossed midnight
        return ( nowtm.tm_hour - earlytm.tm_hour ) >= this->hours;
      }
      else {
        // we're into the next day, so add 24 hours to our now hour
        return ( nowtm.tm_hour + 24 - earlytm.tm_hour ) >= this->hours;
      }
    }
    return (now - earliest ) >= ( this->hours * 60 * 60 * 1000 );
  }
}