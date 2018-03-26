/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "DurationSignalData.h"
#include "DataRow.h"
#include "DurationSpecification.h"

DurationSignalData::DurationSignalData( const std::string& name, const DurationSpecification& spec,
    bool largefilesupport, bool iswave ) :
SignalData( name, largefilesupport, iswave ), duration( spec ) {

}

void DurationSignalData::add( const DataRow& row ) {
  if ( duration.accepts( row.time ) ) {
    SignalData::add( row );
  }
}