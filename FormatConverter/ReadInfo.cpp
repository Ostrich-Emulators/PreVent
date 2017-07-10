/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "ReadInfo.h"
#include "SignalData.h"

ReadInfo::ReadInfo( ) : largefile( false ) {
}

ReadInfo::~ReadInfo( ) {
}

ReadInfo::ReadInfo( const ReadInfo& ) {
}

ReadInfo ReadInfo::operator=(const ReadInfo&) {
}

void ReadInfo::setFileSupport( bool b ) {
  largefile = b;
}

std::map<std::string, std::unique_ptr<SignalData>>&ReadInfo::vitals( ) {
  return vmap;
}

std::map<std::string, std::unique_ptr<SignalData>>&ReadInfo::waves( ) {
  return wmap;
}

std::map<std::string, std::string>& ReadInfo::metadata( ) {
  return metamap;
}

void ReadInfo::addMeta( const std::string& key, const std::string& val ) {
  metamap[key] = val;
}

std::unique_ptr<SignalData>& ReadInfo::addVital( const std::string& name ) {
  if ( 0 == vmap.count( name ) ) {
    vmap.insert( std::make_pair( name,
        std::unique_ptr<SignalData>( new SignalData( name,
        largefile ) ) ) );
  }

  return vmap[name];
}

std::unique_ptr<SignalData>& ReadInfo::addWave( const std::string& name ) {
  if ( 0 == wmap.count( name ) ) {
    wmap.insert( std::make_pair( name,
        std::unique_ptr<SignalData>( new SignalData( name,
        largefile ) ) ) );
  }

  return wmap[name];
}

void ReadInfo::reset( bool signalDataOnly ) {
  vmap.clear( );
  wmap.clear( );

  if ( !signalDataOnly ) {
    metamap.clear( );
  }
}
