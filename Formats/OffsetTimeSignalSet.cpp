/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "OffsetTimeSignalSet.h"
#include "BasicSignalSet.h"
#include "SignalData.h"

const std::string OffsetTimeSignalSet::COLLECTION_OFFSET = "Collection Time Offset";

OffsetTimeSignalSet::OffsetTimeSignalSet( long offset_ms )
: SignalSetWrapper( new BasicSignalSet( ) ), offset_ms( offset_ms ) {
  SignalSetWrapper::setMeta( COLLECTION_OFFSET, std::to_string( offset_ms ) );
}

OffsetTimeSignalSet::OffsetTimeSignalSet( const std::unique_ptr<SignalSet>& w,
    long offset_ms ) : SignalSetWrapper( w ), offset_ms( offset_ms ) {
  SignalSetWrapper::setMeta( COLLECTION_OFFSET, std::to_string( offset_ms ) );
}

OffsetTimeSignalSet::OffsetTimeSignalSet( SignalSet * w, long offset_ms )
: SignalSetWrapper( w ), offset_ms( offset_ms ) {
  SignalSetWrapper::setMeta( COLLECTION_OFFSET, std::to_string( offset_ms ) );
}

OffsetTimeSignalSet::~OffsetTimeSignalSet( ) {
}

std::unique_ptr<SignalData>& OffsetTimeSignalSet::addVital( const std::string& name, bool * added ) {
  bool realadd;
  std::unique_ptr<SignalData>& data = SignalSetWrapper::addVital( name, &realadd );
  if ( realadd ) {
    data->setMeta( COLLECTION_OFFSET, (int) offset_ms );
  }

  if ( nullptr != added ) {
    *added = realadd;
  }

  return data;
}

std::unique_ptr<SignalData>& OffsetTimeSignalSet::addWave( const std::string& name, bool * added ) {
  bool realadd;
  std::unique_ptr<SignalData>& data = SignalSetWrapper::addWave( name, &realadd );
  if ( realadd ) {
    data->setMeta( COLLECTION_OFFSET, (int) offset_ms );
  }

  if ( nullptr != added ) {
    *added = realadd;
  }

  return data;
}

void OffsetTimeSignalSet::offset( long off ) {
  offset_ms = off;
  SignalSetWrapper::setMeta( COLLECTION_OFFSET, std::to_string( offset_ms ) );

  // now reset all signals' offsets
  for ( auto& v : vitals( ) ) {
    v->setMeta( COLLECTION_OFFSET, (int) offset_ms );
  }

  for ( auto& w : waves( ) ) {
    w->setMeta( COLLECTION_OFFSET, (int) offset_ms );
  }
}

void OffsetTimeSignalSet::reset( bool signalDataOnly ) {
  SignalSetWrapper::reset( signalDataOnly );
  offset( offset_ms );
}

void OffsetTimeSignalSet::complete( ) {
  SignalSetWrapper::complete( );
  offset( offset_ms );
}