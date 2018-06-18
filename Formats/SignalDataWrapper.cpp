/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "DataRow.h"
#include "SignalDataWrapper.h"

SignalDataWrapper::SignalDataWrapper( const std::unique_ptr<SignalData>& data )
: signal( data ) {
}

SignalDataWrapper::SignalDataWrapper( SignalData * data )
:signal( std::unique_ptr<SignalData>( data ) ){
}

SignalDataWrapper::~SignalDataWrapper( ) {
}

std::unique_ptr<SignalData> SignalDataWrapper::shallowcopy( bool includedates ) {
  return signal->shallowcopy( includedates );
}

void SignalDataWrapper::moveDataTo( std::unique_ptr<SignalData>& signal ) {
  signal->moveDataTo( signal );
}

void SignalDataWrapper::add( const DataRow& row ) {
  signal->add( row );
}

void SignalDataWrapper::setUom( const std::string& u ) {
  signal->setUom( u );
}

const std::string& SignalDataWrapper::uom( ) const {
  return signal->uom( );
}

int SignalDataWrapper::scale( ) const {
  return signal->scale( );
}

size_t SignalDataWrapper::size( ) const {
  return signal->size( );
}

double SignalDataWrapper::hz( ) const {
  return signal->hz( );
}

const dr_time& SignalDataWrapper::startTime( ) const {
  return signal->startTime( );
}

const dr_time& SignalDataWrapper::endTime( ) const {
  return signal->endTime( );
}

const std::string& SignalDataWrapper::name( ) const {
  return signal->name( );
}

void SignalDataWrapper::setChunkIntervalAndSampleRate( int chunk_ms, int sr ) {
  signal->setChunkIntervalAndSampleRate( chunk_ms, sr );
}

int SignalDataWrapper::readingsPerSample( ) const {
  return signal->readingsPerSample( );
}

void SignalDataWrapper::setMetadataFrom( const SignalData& model ) {
  signal->setMetadataFrom( model );
}

std::unique_ptr<DataRow> SignalDataWrapper::pop( ) {
  return signal->pop( );
}

bool SignalDataWrapper::empty( ) const {
  return signal->empty( );
}

void SignalDataWrapper::setWave( bool wave ) {
  signal->setWave( wave );
}

bool SignalDataWrapper::wave( ) const {
  return signal->wave( );
}

std::map<std::string, std::string>& SignalDataWrapper::metas( ) {
  return signal->metas( );
}

std::map<std::string, int>& SignalDataWrapper::metai( ) {
  return signal->metai( );
}

std::map<std::string, double>& SignalDataWrapper::metad( ) {
  return signal->metad( );
}

const std::map<std::string, std::string>& SignalDataWrapper::metas( ) const {
  return signal->metas( );
}

const std::map<std::string, int>& SignalDataWrapper::metai( ) const {
  return signal->metai( );
}

const std::map<std::string, double>& SignalDataWrapper::metad( ) const {
  return signal->metad( );
}

const std::deque<dr_time> SignalDataWrapper::times( long offset_ms ) const {
  return signal->times( offset_ms );
}

std::vector<std::string> SignalDataWrapper::extras( ) const {
  return signal->extras( );
}

double SignalDataWrapper::highwater( ) const {
  return signal->highwater( );
}

double SignalDataWrapper::lowwater( ) const {
  return signal->lowwater( );
}
