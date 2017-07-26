/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalSet.h"
#include "SignalData.h"

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

std::map<std::string, std::string>& SignalSet::metadata( ) {
  return metamap;
}

void SignalSet::addMeta( const std::string& key, const std::string& val ) {
  metamap[key] = val;
}

std::unique_ptr<SignalData>& SignalSet::addVital( const std::string& name, bool * added ) {
  int cnt = vmap.count( name );
  if ( 0 == cnt ) {
    vmap.insert( std::make_pair( name,
        std::unique_ptr<SignalData>( new SignalData( name,
        largefile ) ) ) );
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
        std::unique_ptr<SignalData>( new SignalData( name,
        largefile ) ) ) );
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
