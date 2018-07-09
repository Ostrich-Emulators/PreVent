/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalSetWrapper.h"

SignalSetWrapper::SignalSetWrapper( const std::unique_ptr<SignalSet>& model ) : set( model ) {
}

SignalSetWrapper::SignalSetWrapper( SignalSet * model ) : set( std::unique_ptr<SignalSet>( model ) ) {
}

SignalSetWrapper::~SignalSetWrapper( ) {
}

std::unique_ptr<SignalData>& SignalSetWrapper::addVital( const std::string& name,
    bool * added ) {
  return set->addVital( name, added );
}

std::unique_ptr<SignalData>& SignalSetWrapper::addWave( const std::string& name,
    bool * added ) {
  return set->addWave( name, added );
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

SignalDataIterator SignalSetWrapper::begin( ) {
  return set->begin( );
}

SignalDataIterator SignalSetWrapper::end( ) {
  return set->end( );
}

SignalDataIterator SignalSetWrapper::vbegin( ) {
  return set->vbegin( );
}

SignalDataIterator SignalSetWrapper::vend( ) {
  return set->vend( );
}

SignalDataIterator SignalSetWrapper::wbegin( ) {
  return set->wbegin( );
}

SignalDataIterator SignalSetWrapper::wend( ) {
  return set->wend( );
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
