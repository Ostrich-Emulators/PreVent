/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalSetWrapper.h"
#include <iostream>

SignalSetWrapper::SignalSetWrapper( const std::unique_ptr<SignalSet>& model )
: set( model.get( ) ), iOwnThisPointer( false ) {
}

SignalSetWrapper::SignalSetWrapper( SignalSet * model ) : set( model ), iOwnThisPointer( true ) {
}

SignalSetWrapper::~SignalSetWrapper( ) {
  if ( iOwnThisPointer ) {
    delete set;
  }
}

void SignalSetWrapper::setMeta( const std::string& key, const std::string& val ) {
  set->setMeta( key, val );
}

const std::map<std::string, std::string>& SignalSetWrapper::metadata( ) const {
  return set->metadata( );
}

std::unique_ptr<SignalData>& SignalSetWrapper::addVital( const std::string& name,
    bool * added ) {
  return set->addVital( name, added );
}

std::unique_ptr<SignalData>& SignalSetWrapper::addWave( const std::string& name,
    bool * added ) {
  return set->addWave( name, added );
}

bool SignalSetWrapper::isLargeFile( ) const {
  return set->isLargeFile( );
}


//std::vector<std::reference_wrapper<const std::unique_ptr<SignalData>>> SignalSetWrapper::vitals( ) const {
//  const SignalSet * x = set.get( );
//  return x->vitals( );
//}
//
//std::vector<std::reference_wrapper<const std::unique_ptr<SignalData>>> SignalSetWrapper::waves( ) const {
//  const SignalSet * x = set.get( );
//  return x->waves( );
//}
//
//std::vector<std::reference_wrapper<std::unique_ptr<SignalData>>> SignalSetWrapper::vitals( ) {
//  return set->vitals( );
//}
//
//std::vector<std::reference_wrapper<std::unique_ptr<SignalData>>> SignalSetWrapper::waves( ) {
//  return set->waves( );
//}

std::vector<std::unique_ptr<SignalData>>&SignalSetWrapper::vitals( ) {
  return set->vitals( );
}

std::vector<std::unique_ptr<SignalData>>&SignalSetWrapper::waves( ) {
  return set->waves( );
}

const std::vector<std::unique_ptr<SignalData>>&SignalSetWrapper::vitals( ) const {
  return set->vitals( );
}

const std::vector<std::unique_ptr<SignalData>>&SignalSetWrapper::waves( ) const {
  return set->waves( );
}

void SignalSetWrapper::reset( bool signalDataOnly ) {
  return set->reset( signalDataOnly );
}

dr_time SignalSetWrapper::earliest( const TimeCounter& tc ) const {
  return set->earliest( tc );
}

dr_time SignalSetWrapper::latest( const TimeCounter& tc ) const {
  return set->latest( tc );
}
