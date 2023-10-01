/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Options.h"

namespace FormatConverter{
  std::map<OptionsKey, std::string> Options::map;

  Options::Options( ) { }

  Options::~Options( ) { }

  bool Options::isset( OptionsKey key ) {
    return 0 < map.count( key );
  }

  void Options::set( OptionsKey key, const std::string& val ) {
    map.erase( key );
    map.insert( std::make_pair( key, val ) );
  }

  void Options::set( OptionsKey key, bool val ) {
    map.erase( key );
    map.insert( std::make_pair( key, val ? "T" : "F" ) );
  }

  void Options::set( OptionsKey key, const dr_time& t ) {
    map.erase( key );
    map.insert( std::make_pair( key, std::to_string( t ) ) );
  }

  std::string Options::get( OptionsKey key ) {
    return isset( key )
        ? map[key]
        : "";
  }

  bool Options::asBool( OptionsKey key ) {
    std::string val = get( key );
    return ( "T" == val );
  }

  dr_time Options::asTime( OptionsKey key ) {
    return isset( key )
        ? static_cast<dr_time> ( std::stoul( get( key ) ) )
        : 0;
  }
}