/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "BasicSignalSet.h"
#include "BasicSignalData.h"
#include "SignalUtils.h"
#include "DurationSpecification.h"

#include <limits>
#include <iostream>

BasicSignalSet::BasicSignalSet() {
  setMeta( SignalData::TIMEZONE, "UTC" );
}

BasicSignalSet::BasicSignalSet( const BasicSignalSet& ) {
  setMeta( SignalData::TIMEZONE, "UTC" );
}

BasicSignalSet::~BasicSignalSet( ) {
}

BasicSignalSet BasicSignalSet::operator=(const BasicSignalSet&) {
}

std::vector<std::unique_ptr<SignalData>>&BasicSignalSet::vitals( ) {
  return vits;
}

std::vector<std::unique_ptr<SignalData>>&BasicSignalSet::waves( ) {
  return wavs;
}

const std::vector<std::unique_ptr<SignalData>>&BasicSignalSet::vitals( ) const {
  return vits;
}

const std::vector<std::unique_ptr<SignalData>>&BasicSignalSet::waves( ) const {
  return wavs;
}

dr_time BasicSignalSet::earliest( const TimeCounter& type ) const {
  dr_time early = std::numeric_limits<dr_time>::max( );

  if ( TimeCounter::VITAL == type || TimeCounter::EITHER == type ) {
    early = SignalUtils::firstlast( vits );
  }
  if ( TimeCounter::WAVE == type || TimeCounter::EITHER == type ) {
    dr_time w = SignalUtils::firstlast( wavs );
    if ( w < early ) {
      early = w;
    }
  }

  return early;
}

dr_time BasicSignalSet::latest( const TimeCounter& type ) const {
  dr_time last = 0;

  if ( TimeCounter::VITAL == type || TimeCounter::EITHER == type ) {
    SignalUtils::firstlast( vits, nullptr, &last );
  }

  if ( TimeCounter::WAVE == type || TimeCounter::EITHER == type ) {
    dr_time w;
    SignalUtils::firstlast( wavs, nullptr, &w );
    if ( w > last ) {
      last = w;
    }
  }

  return last;
}

std::unique_ptr<SignalData> BasicSignalSet::createSignalData( const std::string& name, bool iswave ) {
  return std::unique_ptr<SignalData>( new BasicSignalData( name, iswave ) );
}

std::unique_ptr<SignalData>& BasicSignalSet::addVital( const std::string& name, bool * added ) {
  for ( auto& x : vits ) {
    if ( x->name( ) == name ) {
      if ( nullptr != added ) {
        *added = false;
        return x;
      }
    }
  }

  vits.push_back( createSignalData( name, false ) );

  if ( nullptr != added ) {
    *added = true;
  }

  return vits[vits.size( ) - 1];
}

std::unique_ptr<SignalData>& BasicSignalSet::addWave( const std::string& name, bool * added ) {
  for ( auto& x : wavs ) {
    if ( x->name( ) == name ) {
      if ( nullptr != added ) {
        *added = false;
        return x;
      }
    }
  }

  wavs.push_back( createSignalData( name, false ) );

  if ( nullptr != added ) {
    *added = true;
  }

  return wavs[wavs.size( ) - 1];
}

void BasicSignalSet::reset( bool signalDataOnly ) {
  vits.clear( );
  wavs.clear( );
  SignalSet::reset( signalDataOnly );
}