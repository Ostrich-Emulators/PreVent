/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalSetWrapper.h"
#include <iostream>
#include "SignalData.h"

namespace FormatConverter{

  SignalSetWrapper::SignalSetWrapper( const std::unique_ptr<SignalSet>& model )
      : set( model.get( ) ), iOwnThisPointer( false ) { }

  SignalSetWrapper::SignalSetWrapper( SignalSet * model ) : set( model ), iOwnThisPointer( true ) { }

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

  SignalData * SignalSetWrapper::addVital( const std::string& name, bool * added ) {
    return set->addVital( name, added );
  }

  SignalData * SignalSetWrapper::addWave( const std::string& name, bool * added ) {
    return set->addWave( name, added );
  }

  std::vector<SignalData *> SignalSetWrapper::vitals( ) const {
    return set->vitals( );
  }

  std::vector<SignalData *> SignalSetWrapper::waves( ) const {
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

  const std::map<long, dr_time>& SignalSetWrapper::offsets( ) const {
    return set->offsets( );

  }

  void SignalSetWrapper::addOffset( long seg, dr_time time ) {
    set->addOffset( seg, time );
  }

  void SignalSetWrapper::clearOffsets( ) {
    set->clearOffsets( );
  }

  std::vector<SignalData *> SignalSetWrapper::allsignals( ) const {
    return set->allsignals( );
  }

  void SignalSetWrapper::clearMetas( ) {
    set->clearMetas( );
  }

  void SignalSetWrapper::setMetadataFrom( const SignalSet& target ) {
    set->setMetadataFrom( target );
  }

  void SignalSetWrapper::complete( ) {
    set->complete( );
  }

  void SignalSetWrapper::addAuxillaryData( const std::string& name, const TimedData& data  ) {
    set->addAuxillaryData( name, data );
  }

  std::map<std::string, std::vector<TimedData>> SignalSetWrapper::auxdata( ) {
    return set->auxdata();
  }

  std::unique_ptr<SignalData> SignalSetWrapper::_createSignalData( const std::string& name,
        bool iswave , void * extra ){
    return set->_createSignalData( name, iswave, extra );
  }
}