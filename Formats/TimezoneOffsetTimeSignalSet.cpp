/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TimezoneOffsetTimeSignalSet.h"
#include "BasicSignalSet.h"
#include "SignalData.h"

const std::string TimezoneOffsetTimeSignalSet::COLLECTION_TIMEZONE = "Collection Timezone";

TimezoneOffsetTimeSignalSet::TimezoneOffsetTimeSignalSet( )
: OffsetTimeSignalSet( ), tz_calculated( false ) {
}

TimezoneOffsetTimeSignalSet::TimezoneOffsetTimeSignalSet( const std::unique_ptr<SignalSet>& w )
: OffsetTimeSignalSet( w ), tz_calculated( false ) {
}

TimezoneOffsetTimeSignalSet::TimezoneOffsetTimeSignalSet( SignalSet * w )
: OffsetTimeSignalSet( w ), tz_calculated( false ) {
}

TimezoneOffsetTimeSignalSet::~TimezoneOffsetTimeSignalSet( ) {
}

void TimezoneOffsetTimeSignalSet::reset( bool signalDataOnly ) {
  SignalSetWrapper::reset( signalDataOnly );

  // now reset all signals' offsets
  for ( auto& v : vitals( ) ) {
    v->setMeta( COLLECTION_TIMEZONE, tz_name );
  }

  for ( auto& w : waves( ) ) {
    w->setMeta( COLLECTION_TIMEZONE, tz_name );
  }
}

void TimezoneOffsetTimeSignalSet::complete( ) {
  if ( !tz_calculated ) {
    tz_calculated = true;

    time_t first = earliest( ) / 1000;
    tm * reftm = localtime( &first );
    tz_name = reftm->tm_zone;
    offset( reftm->tm_gmtoff * 1000 );
  }
  OffsetTimeSignalSet::complete( );

  setMeta( COLLECTION_TIMEZONE, tz_name );

  // now reset all signals' offsets
  for ( auto& v : vitals( ) ) {
    v->setMeta( COLLECTION_TIMEZONE, tz_name );
  }

  for ( auto& w : waves( ) ) {
    w->setMeta( COLLECTION_TIMEZONE, tz_name );
  }
}
