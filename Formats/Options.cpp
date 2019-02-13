/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Options.h"


std::map<OptionsKey, std::string> Options::map;

Options::Options( ) {
}

Options::~Options( ) {
}

void Options::set( OptionsKey key, const std::string& val ) {
  map.erase( key );
  map.insert( std::make_pair( key, val ) );
}

void Options::set( OptionsKey key, bool val ) {
  map.erase( key );
  map.insert( std::make_pair( key, val ? "T" : "F" ) );
}

std::string Options::get( OptionsKey key ) {
  if ( 0 < map.count( key ) ) {
    return map[key];
  }
  return "";
}

bool Options::asBool( OptionsKey key ) {
  std::string val = get( key );
  return ( "T" == val );
}