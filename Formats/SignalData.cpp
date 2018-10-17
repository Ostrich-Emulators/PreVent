/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataSetDataCache.cpp
 * Author: ryan
 * 
 * Created on August 3, 2016, 7:47 AM
 */

#include "SignalData.h"
#include "DataRow.h"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <limits>
#include <queue>

const std::string SignalData::SCALE = "Scale";
const std::string SignalData::UOM = "Unit of Measure";
const std::string SignalData::MSM = "Missing Value Marker";
const std::string SignalData::TIMEZONE = "Timezone";
const std::string SignalData::BUILD_NUM = "Build Number";
const std::string SignalData::STARTTIME = "Start Time";

const std::string SignalData::CHUNK_INTERVAL_MS = "Sample Period (ms)";
const std::string SignalData::READINGS_PER_CHUNK = "Readings Per Sample";

const std::string SignalData::LABEL = "Data Label";
const short SignalData::MISSING_VALUE = -32768;
const std::string SignalData::MISSING_VALUESTR = std::to_string( SignalData::MISSING_VALUE );

SignalData::SignalData( ) {
}

SignalData::~SignalData( ) {
}

void SignalData::moveDataTo( std::unique_ptr<SignalData>& dest ) {
  size_t count = size( );
  for ( size_t i = 0; i < count; i++ ) {
    std::unique_ptr<DataRow> dr = pop( );
    dest->add( *dr );
  }
}

double SignalData::hz( ) const {
  double ratio = 1000.0 / (double) metai( ).at( SignalData::CHUNK_INTERVAL_MS );
  return ratio * (double) metai( ).at( SignalData::READINGS_PER_CHUNK );
}

bool SignalData::empty( ) const {
  return ( 0 == size( ) );
}

void SignalData::setUom( const std::string& u ) {
  setMeta( UOM, u );
}

const std::string& SignalData::uom( ) const {
  return metas( ).at( UOM );
}

int SignalData::scale( ) const {
  return metai( ).at( SCALE );
}

void SignalData::scale( int x ) {
  setMeta( SCALE, x );
}

int SignalData::readingsPerSample( ) const {
  return metai( ).at( READINGS_PER_CHUNK );
}

void SignalData::setChunkIntervalAndSampleRate( int chunktime_ms, int samplerate ) {
  setMeta( CHUNK_INTERVAL_MS, chunktime_ms );
  setMeta( READINGS_PER_CHUNK, samplerate );
}
