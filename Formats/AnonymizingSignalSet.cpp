/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "AnonymizingSignalSet.h"
#include "BasicSignalSet.h"

class AnonymizingSignalData : public SignalDataWrapper {
public:
  AnonymizingSignalData( SignalData * data, dr_time& firsttime );
  AnonymizingSignalData( const std::unique_ptr<SignalData>& data, dr_time& firsttime );
  virtual ~AnonymizingSignalData( );

  virtual void add( const DataRow& row ) override;

private:
  dr_time& firsttime;
};

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

AnonymizingSignalData::AnonymizingSignalData( SignalData * data, dr_time& first )
: SignalDataWrapper( data ), firsttime( first ) {
}

AnonymizingSignalData::AnonymizingSignalData( const std::unique_ptr<SignalData>& data, dr_time& first )
: SignalDataWrapper( data ), firsttime( first ) {
}

AnonymizingSignalData::~AnonymizingSignalData( ) {
}

void AnonymizingSignalData::add( const DataRow& row ) {
  if ( 0 == firsttime ) {
    firsttime = row.time;
  }
  DataRow newrow( row );
  newrow.time -= firsttime;
  SignalDataWrapper::add( newrow );
}