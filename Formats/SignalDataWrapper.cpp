/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "DataRow.h"
#include "SignalDataWrapper.h"
#include <iostream>

SignalDataWrapper::SignalDataWrapper( const std::unique_ptr<SignalData>& data )
: signal( data.get( ) ), iOwnThisPtr( false ) {
}

SignalDataWrapper::SignalDataWrapper( SignalData * data )
: signal( data ), iOwnThisPtr( true ) {
}

SignalDataWrapper::~SignalDataWrapper( ) {
  if ( iOwnThisPtr ) {
    delete signal;
  }
}

size_t SignalDataWrapper::inmemsize( ) const{
  return signal->inmemsize();
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

dr_time SignalDataWrapper::startTime( ) const {
  return signal->startTime( );
}

dr_time SignalDataWrapper::endTime( ) const {
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

int SignalDataWrapper::chunkInterval( ) const {
  return signal->chunkInterval( );
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

void SignalDataWrapper::setMeta( const std::string& key, const std::string& val ) {
  signal->setMeta( key, val );
}

void SignalDataWrapper::setMeta( const std::string& key, int val ) {
  signal->setMeta( key, val );
}

void SignalDataWrapper::setMeta( const std::string& key, double val ) {
  signal->setMeta( key, val );
}

void SignalDataWrapper::erases( const std::string& key ) {
  signal->erases( key );
}

void SignalDataWrapper::erasei( const std::string& key ) {
  signal->erasei( key );
}

void SignalDataWrapper::erased( const std::string& key ) {
  signal->erased( key );
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

const std::deque<dr_time> SignalDataWrapper::times( ) const {
  return signal->times( );
}

std::vector<std::string> SignalDataWrapper::extras( ) const {
  return signal->extras( );
}

void SignalDataWrapper::extras( const std::string& ext ) {
  signal->extras( ext );
}

double SignalDataWrapper::highwater( ) const {
  return signal->highwater( );
}

double SignalDataWrapper::lowwater( ) const {
  return signal->lowwater( );
}

void SignalDataWrapper::recordEvent( const std::string& eventtype, const dr_time& time ) {
  signal->recordEvent( eventtype, time );
}

std::vector<std::string> SignalDataWrapper::eventtypes( ) {
  return signal->eventtypes( );
}

std::vector<dr_time> SignalDataWrapper::events( const std::string& type ) {
  return signal->events( type );
}
