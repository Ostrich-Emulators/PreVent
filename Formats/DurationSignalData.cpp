/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "DurationSignalData.h"
#include "DataRow.h"
#include "DurationSpecification.h"

DurationSignalData::DurationSignalData( std::unique_ptr<SignalData> data, const DurationSpecification& dur )
: signal( std::move( data ) ), spec( dur ) {
}

DurationSignalData::~DurationSignalData( ) {
}

std::unique_ptr<SignalData> DurationSignalData::shallowcopy( bool includedates ) {
  return signal->shallowcopy( includedates );
}

void DurationSignalData::moveDataTo( std::unique_ptr<SignalData>& signal ) {
  signal->moveDataTo( signal );
}

void DurationSignalData::add( const DataRow& row ) {
  if ( spec.accepts( row.time ) ) {
    signal->add( row );
  }
}

void DurationSignalData::setUom( const std::string& u ) {
  signal->setUom( u );
}

const std::string& DurationSignalData::uom( ) const {
  return signal->uom( );
}

int DurationSignalData::scale( ) const {
  return signal->scale( );
}

size_t DurationSignalData::size( ) const {
  return signal->size( );
}

double DurationSignalData::hz( ) const {
  return signal->hz( );
}

const dr_time& DurationSignalData::startTime( ) const {
  return signal->startTime( );
}

const dr_time& DurationSignalData::endTime( ) const {
  return signal->endTime( );
}

const std::string& DurationSignalData::name( ) const {
  return signal->name( );
}

void DurationSignalData::setValuesPerDataRow( int vpr ) {
  signal->setValuesPerDataRow( vpr );
}

int DurationSignalData::valuesPerDataRow( ) const {
  return signal->valuesPerDataRow( );
}

void DurationSignalData::setMetadataFrom( const SignalData& model ) {
  signal->setMetadataFrom( model );
}

std::unique_ptr<DataRow> DurationSignalData::pop( ) {
  return signal->pop( );
}

bool DurationSignalData::empty( ) const {
  return signal->empty( );
}

void DurationSignalData::setWave( bool wave ) {
  signal->setWave( wave );
}

bool DurationSignalData::wave( ) const {
  return signal->wave( );
}

std::map<std::string, std::string>& DurationSignalData::metas( ) {
  return signal->metas( );
}

std::map<std::string, int>& DurationSignalData::metai( ) {
  return signal->metai( );
}

std::map<std::string, double>& DurationSignalData::metad( ) {
  return signal->metad( );
}

const std::map<std::string, std::string>& DurationSignalData::metas( ) const {
  return signal->metas( );
}

const std::map<std::string, int>& DurationSignalData::metai( ) const {
  return signal->metai( );
}

const std::map<std::string, double>& DurationSignalData::metad( ) const {
  return signal->metad( );
}

const std::deque<dr_time>& DurationSignalData::times( ) const {
  return signal->times( );
}

std::vector<std::string> DurationSignalData::extras( ) const {
  return signal->extras( );
}
