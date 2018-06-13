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

const std::string SignalData::CHUNK_INTERVAL_MS = "Sample Period (ms)";
const std::string SignalData::READINGS_PER_CHUNK = "Readings Per Sample";

const std::string SignalData::LABEL = "Data Label";
const short SignalData::MISSING_VALUE = -32768;
const std::string SignalData::MISSING_VALUESTR = std::to_string( SignalData::MISSING_VALUE );

SignalData::SignalData( ) {
  scale( 1 );
  setChunkIntervalAndSampleRate( 2000, 1 );
  setUom( "Uncalib" );
  metadatai[SignalData::MSM] = SignalData::MISSING_VALUE;
  metadatas[SignalData::TIMEZONE] = "UTC";
}

SignalData::SignalData( const SignalData& orig ) : metadatas( orig.metadatas ),
metadatai( orig.metadatai ), metadatad( orig.metadatad ),
extrafields( orig.extrafields.begin( ), orig.extrafields.end( ) ) {
}

void SignalData::moveDataTo( std::unique_ptr<SignalData>& dest ) {
  size_t count = size( );
  for ( size_t i = 0; i < count; i++ ) {
    std::unique_ptr<DataRow> dr = pop( );
    dest->add( *dr );
  }
}

std::map<std::string, std::string>& SignalData::metas( ) {
  return metadatas;
}

std::map<std::string, int>& SignalData::metai( ) {
  return metadatai;
}

std::map<std::string, double>& SignalData::metad( ) {
  return metadatad;
}

const std::map<std::string, std::string>& SignalData::metas( ) const {
  return metadatas;
}

const std::map<std::string, int>& SignalData::metai( ) const {
  return metadatai;
}

const std::map<std::string, double>& SignalData::metad( ) const {
  return metadatad;
}

double SignalData::hz( ) const {
  double ratio = 1000.0 / (double) metadatai.at( SignalData::CHUNK_INTERVAL_MS );
  return ratio * (double)metadatai.at( SignalData::READINGS_PER_CHUNK );
}

std::vector<std::string> SignalData::extras( ) const {
  return std::vector<std::string>( extrafields.begin( ), extrafields.end( ) );
}

void SignalData::extras( const std::string& ext ) {
  extrafields.insert( ext );
}

bool SignalData::empty( ) const {
  return ( 0 == size( ) );
}

SignalData::~SignalData( ) {
}

void SignalData::setUom( const std::string& u ) {
  metadatas[UOM] = u;
}

const std::string& SignalData::uom( ) const {
  return metadatas.at( UOM );
}

int SignalData::scale( ) const {
  return metadatai.at( SCALE );
}

void SignalData::scale( int x ) {
  metadatai[SCALE] = x;
}

int SignalData::readingsPerSample( ) const {
  return metadatai.at( READINGS_PER_CHUNK );
}

void SignalData::setChunkIntervalAndSampleRate( int chunktime_ms, int samplerate ) {
  metadatai[CHUNK_INTERVAL_MS] = chunktime_ms;
  metadatai[READINGS_PER_CHUNK] = samplerate;
}

void SignalData::setMetadataFrom( const SignalData& model ) {
  metadatai.clear( );
  metadatai.insert( model.metai( ).begin( ), model.metai( ).end( ) );

  metadatas.clear( );
  metadatas.insert( model.metas( ).begin( ), model.metas( ).end( ) );

  metadatad.clear( );
  metadatad.insert( model.metad( ).begin( ), model.metad( ).end( ) );

  extrafields.clear( );
  std::vector<std::string> vec = model.extras( );
  extrafields.insert( vec.begin( ), vec.end( ) );
}