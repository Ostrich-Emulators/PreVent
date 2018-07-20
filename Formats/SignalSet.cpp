/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalSet.h"
#include "BasicSignalData.h"
#include "SignalUtils.h"
#include "DurationSpecification.h"

#include <limits>
#include <iostream>

SignalSet::SignalSet() {
}

SignalSet::~SignalSet( ) {
}

void SignalSet::reset( bool signalDataOnly ) {
  if ( !signalDataOnly ) {
    metamap.clear( );
  }
}

std::vector<std::reference_wrapper<const std::unique_ptr<SignalData>>> SignalSet::allsignals( ) const {
  std::vector<std::reference_wrapper<const std::unique_ptr < SignalData>>> vec;

  for ( const auto& m : vitals( ) ) {
    vec.push_back( std::cref( m ) );
  }
  for ( const auto& m : waves( ) ) {
    vec.push_back( std::cref( m ) );
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

const std::map<std::string, std::string>& SignalSet::metadata( ) const {
  return metamap;
}

void SignalSet::setMeta( const std::string& key, const std::string & val ) {
  metamap[key] = val;
}

void SignalSet::clearMetas(){
  metamap.clear();
}

const std::map<long, dr_time>& SignalSet::offsets( ) const {
  return segs;
}

void SignalSet::addOffset( long seg, dr_time time ) {
  segs[seg] = time;
}

void SignalSet::clearOffsets( ) {
  segs.clear( );
}
