/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalSet.h"
#include "SignalData.h"
#include "SignalUtils.h"

#include <limits>

SignalSet::SignalSet( ) : largefile( false ) {
}

SignalSet::~SignalSet( ) {
}

SignalSet::SignalSet( const SignalSet& ) {
}

SignalSet SignalSet::operator=(const SignalSet&) {
}

void SignalSet::setFileSupport( bool b ) {
  largefile = b;
}

std::map<std::string, std::unique_ptr<SignalData>>&SignalSet::vitals( ) {
  return vmap;
}

std::map<std::string, std::unique_ptr<SignalData>>&SignalSet::waves( ) {
  return wmap;
}

const std::map<std::string, std::unique_ptr<SignalData>>&SignalSet::vitals( ) const {
  return vmap;
}

const std::map<std::string, std::unique_ptr<SignalData>>&SignalSet::waves( ) const {
  return wmap;
}

std::vector<std::reference_wrapper<const std::unique_ptr<SignalData>>> SignalSet::allsignals( ) const {
  std::vector<std::reference_wrapper<const std::unique_ptr < SignalData>>> vec;

  for ( const auto& m : wmap ) {
    const auto& w = m.second;
    vec.push_back( std::cref( w ) );
  }
  for ( const auto& m : vmap ) {
    const auto& w = m.second;
    vec.push_back( std::cref( w ) );
  }

  return vec;
}

void SignalSet::setMetadataFrom( const SignalSet& src ) {
  if ( this != &src ) {
    metamap.clear( );
    metamap.insert( src.metadata( ).begin( ), src.metadata( ).end( ) );

    segs.clear( );
    segs.insert( src.offsets( ).begin( ), src.offsets( ).end( ) );
  }
}

dr_time SignalSet::earliest( const TimeCounter& type ) const {
  dr_time early = std::numeric_limits<dr_time>::max( );

  if ( TimeCounter::VITAL == type || TimeCounter::EITHER == type ) {
    early = SignalUtils::firstlast( vmap );
  }
  if ( TimeCounter::WAVE == type || TimeCounter::EITHER == type ) {
    dr_time w = SignalUtils::firstlast( wmap );
    if ( w < early ) {
      early = w;
    }
  }

  return early;
}

dr_time SignalSet::latest( const TimeCounter& type ) const {
  dr_time last = 0;

  if ( TimeCounter::VITAL == type || TimeCounter::EITHER == type ) {
    SignalUtils::firstlast( vmap, nullptr, &last );
  }

  if ( TimeCounter::WAVE == type || TimeCounter::EITHER == type ) {
    dr_time w;
    SignalUtils::firstlast( wmap, nullptr, &w );
    if ( w > last ) {
      last = w;
    }
  }

  return last;
}

std::map<std::string, std::string>& SignalSet::metadata( ) {
  return metamap;
}

const std::map<std::string, std::string>& SignalSet::metadata( ) const {
  return metamap;
}

void SignalSet::addMeta( const std::string& key, const std::string & val ) {
  metamap[key] = val;
}

std::unique_ptr<SignalData>& SignalSet::addVital( const std::string& name, bool * added ) {
  int cnt = vmap.count( name );
  if ( 0 == cnt ) {
    vmap.insert( std::make_pair( name,
        std::unique_ptr<SignalData>( new SignalData( name, largefile ) ) ) );
  }

  if ( NULL != added ) {
    *added = ( 0 == cnt );
  }

  return vmap[name];
}

std::unique_ptr<SignalData>& SignalSet::addWave( const std::string& name, bool * added ) {
  int cnt = wmap.count( name );
  if ( 0 == cnt ) {
    wmap.insert( std::make_pair( name,
        std::unique_ptr<SignalData>( new SignalData( name, largefile, true ) ) ) );
  }

  if ( NULL != added ) {

    *added = ( 0 == cnt );
  }

  return wmap[name];
}

void SignalSet::reset( bool signalDataOnly ) {
  vmap.clear( );
  wmap.clear( );

  if ( !signalDataOnly ) {
    metamap.clear( );
  }
}

const std::map<long, dr_time>& SignalSet::offsets( ) const {
  return segs;
}

void SignalSet::addOffset( long seg, dr_time time ) {
  segs[seg] = time;
}

void SignalSet::clearOffsets(){
  segs.clear();
}
