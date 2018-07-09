/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "LocaltimeSignalData.h"
#include "DataRow.h"

LocaltimeSignalData::LocaltimeSignalData( SignalData * data, const std::string& tzname, const long tz_offset_ms )
: SignalDataWrapper( data ), tzname( tzname ), tz_offset_ms( tz_offset_ms ) {
}

LocaltimeSignalData::LocaltimeSignalData( const std::unique_ptr<SignalData>& data, const std::string& tzname,
    const long tz_offset_ms ) : SignalDataWrapper( data ), tzname( tzname ), tz_offset_ms( tz_offset_ms ) {
}

LocaltimeSignalData::~LocaltimeSignalData( ) {
}

dr_time LocaltimeSignalData::startTime( ) const {
  return SignalDataWrapper::startTime( ) + tz_offset_ms;

}

dr_time LocaltimeSignalData::endTime( ) const {
  return SignalDataWrapper::endTime( ) + tz_offset_ms;
}

const std::deque<dr_time> LocaltimeSignalData::times( ) const {
  std::deque<dr_time> ret;
  for ( auto t : SignalDataWrapper::times( ) ) {
    dr_time tt = t + tz_offset_ms;
    ret.push_back( tt );
  }

  return ret;
}
