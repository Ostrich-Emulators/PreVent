/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "AnonymizingSignalSet.h"
#include "BasicSignalSet.h"
#include "AnonymizingSignalData.h"

AnonymizingSignalSet::AnonymizingSignalSet( )
: SignalSetWrapper( new BasicSignalSet( ) ), firsttime( 0 ) {
}

AnonymizingSignalSet::AnonymizingSignalSet( const std::unique_ptr<SignalSet>& w )
: SignalSetWrapper( w ), firsttime( 0 ) {
}

AnonymizingSignalSet::AnonymizingSignalSet( SignalSet * w )
: SignalSetWrapper( w ), firsttime( 0 ) {
}

AnonymizingSignalSet::~AnonymizingSignalSet( ) {
}

std::unique_ptr<SignalData>& AnonymizingSignalSet::addVital( const std::string& name, bool * added ) {
  std::unique_ptr<SignalData>& data = SignalSetWrapper::addVital( name, added );
  if ( nullptr != added && *added ) {
    data.reset( new AnonymizingSignalData( data.release( ), firsttime ) );
  }
  return data;
}

std::unique_ptr<SignalData>& AnonymizingSignalSet::addWave( const std::string& name, bool * added ) {
  std::unique_ptr<SignalData>& data = SignalSetWrapper::addWave( name, added );
  if ( nullptr != added && *added ) {
    data.reset( new AnonymizingSignalData( data.release( ), firsttime ) );
  }
  return data;
}

void AnonymizingSignalSet::setMeta( const std::string& key, const std::string& val ) {
  if ( !( "Patient Name" == key || "MRN" == key || "Unit" == key || "Bed" == key ) ) {
    SignalSetWrapper::setMeta( key, val );
  }
}