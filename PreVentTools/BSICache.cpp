/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "BSICache.h"

namespace FormatConverter{

  BSICache::BSICache( const std::string& name ) : _name( name ) { }

  BSICache::~BSICache(){
  }

  void BSICache::name( const std::string& name ) {
    _name = name;
  }

  const std::string& BSICache::name( ) const {
    return _name;
  }

}