/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "OffsetTimeSignalSet.h"
#include "BasicSignalSet.h"
#include "OffsetTimeSignalData.h"

#include <iostream>

OffsetTimeSignalSet::OffsetTimeSignalSet( const std::string&name, long offset_ms )
: SignalSetWrapper( new BasicSignalSet( ) ), tz_name( name ), tz_offset_ms( offset_ms ) {
  set->addMeta( SignalData::TIMEZONE, tz_name );
}

OffsetTimeSignalSet::OffsetTimeSignalSet( const std::unique_ptr<SignalSet>& w,
    const std::string& tzname, long offset_ms ) : SignalSetWrapper( w ), tz_name( tzname ),
tz_offset_ms( offset_ms ) {
  set->addMeta( SignalData::TIMEZONE, tz_name );
}

OffsetTimeSignalSet::OffsetTimeSignalSet( SignalSet * w, const std::string& tzname, long offset_ms )
: SignalSetWrapper( w ), tz_name( tzname ), tz_offset_ms( offset_ms ) {
  set->addMeta( SignalData::TIMEZONE, tz_name );
}

OffsetTimeSignalSet::~OffsetTimeSignalSet( ) {
}

std::unique_ptr<SignalData>& OffsetTimeSignalSet::addVital( const std::string& name, bool * added ) {
  std::unique_ptr<SignalData>& data = set->addVital( name, added );
  std::cout<<"offset: "<<data.get()<<std::endl;
  SignalData * sd = data.release();
  data.reset( new OffsetTimeSignalData( sd, tz_name, tz_offset_ms ) );
  std::cout<<"new offset: "<<data.get()<<std::endl;
  return data;
}

std::unique_ptr<SignalData>& OffsetTimeSignalSet::addWave( const std::string& name, bool * added ) {
  std::unique_ptr<SignalData>& data = set->addWave( name, added );
  data.reset( new OffsetTimeSignalData( data.release( ), tz_name, tz_offset_ms ) );
  return data;
}