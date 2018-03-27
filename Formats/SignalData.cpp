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

const std::string SignalData::HERTZ = "Sample Frequency (Hz)";
const std::string SignalData::SCALE = "Scale";
const std::string SignalData::UOM = "Unit of Measure";
const std::string SignalData::MSM = "Missing Value Marker";
const std::string SignalData::TIMEZONE = "Timezone";
const std::string SignalData::VALS_PER_DR = "Readings Per Time";
const short SignalData::MISSING_VALUE = -32768;

SignalData::SignalData( ) {
  scale( 1 );
  setValuesPerDataRow( 1 );
  setUom( "Uncalib" );
}

SignalData::SignalData( const SignalData& orig ) : metadatas( orig.metadatas ),
metadatai( orig.metadatai ), metadatad( orig.metadatad ) {
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
  return ( 0 == metadatad.count( SignalData::HERTZ )
          ? 1
          : metadatad.at( SignalData::HERTZ ) );
}

std::vector<std::string> SignalData::extras( ) const {
  return std::vector<std::string>( extrafields.begin( ), extrafields.end( ) );
}

bool SignalData::empty( ) const {
  return ( 0 == size( ) );
}

SignalData::~SignalData( ) {}

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

void SignalData::setValuesPerDataRow( int x ) {
  metadatai[VALS_PER_DR] = x;
}

int SignalData::valuesPerDataRow( ) const {
  return metadatai.at( VALS_PER_DR );
}

void SignalData::setMetadataFrom( const SignalData& model ) {
  metadatai.clear( );
  metadatai.insert( model.metai( ).begin( ), model.metai( ).end( ) );

  metadatas.clear( );
  metadatas.insert( model.metas( ).begin( ), model.metas( ).end( ) );

  metadatad.clear( );
  metadatad.insert( model.metad( ).begin( ), model.metad( ).end( ) );
}