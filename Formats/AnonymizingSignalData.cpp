/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "AnonymizingSignalData.h"
#include "DataRow.h"

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