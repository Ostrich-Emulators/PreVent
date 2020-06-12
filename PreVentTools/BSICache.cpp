/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "BSICache.h"
#include "SignalUtils.h"
#include <iostream>
#include "Options.h"

namespace FormatConverter{
  const size_t BSICache::DEFAULT_CACHE_LIMIT = 1024 * 384; // totally arbitrary, about 3MB data

  BSICache::BSICacheIterator::BSICacheIterator( BSICache * o, size_t cnt )
      : owner( o ), idx( cnt ) { }

  BSICache::BSICacheIterator& BSICache::BSICacheIterator::operator=(const BSICacheIterator& orig ) {
    if ( this != &orig ) {
      this->owner = orig.owner;
      this->idx = orig.idx;
    }
    return *this;
  }

  BSICache::BSICacheIterator::~BSICacheIterator( ) { }

  BSICache::BSICacheIterator& BSICache::BSICacheIterator::operator++( ) {
    idx++;
    return *this;
  }

  BSICache::BSICacheIterator BSICache::BSICacheIterator::operator++(int) {
    auto tmp = BSICacheIterator( this->owner, idx );
    idx++;
    return tmp;
  }

  bool BSICache::BSICacheIterator::operator==( BSICacheIterator& other ) const {
    return ( this->owner == other.owner && this->idx == other.idx );
  }

  bool BSICache::BSICacheIterator::operator!=( BSICacheIterator& other ) const {
    return !operator==( other );
  }

  double BSICache::BSICacheIterator::operator*( ) {
    return owner->value_at( idx );
  }

  BSICache::BSICache( const std::string& name ) : cache( FormatConverter::Options::asBool( FormatConverter::OptionsKey::NOCACHE ) ? nullptr : SignalUtils::tmpf( ) ),
      sizer( 0 ), memrange( 0, 0 ), dirty( false ), _name( name ) { }

  BSICache::~BSICache( ) {
    if ( nullptr != cache ) {
      std::fclose( cache );
      cache = nullptr;
    }
  }

  size_t BSICache::size( ) const {
    return sizer;
  }

  BSICache::BSICacheIterator BSICache::begin( ) {
    return BSICacheIterator( this, 0 );
  }

  BSICache::BSICacheIterator BSICache::end( ) {
    return BSICacheIterator( this, sizer + 1 );
  }

  void BSICache::push_back( const std::vector<double>& vec ) {
    values.insert( values.end( ), vec.begin( ), vec.end( ) );
    sizer += vec.size( );
    memrange.second += vec.size( );
    dirty = true;
    cache_if_needed( );
  }

  void BSICache::push_back( double t ) {
    values.push_back( t );
    sizer++;
    memrange.second++;
    dirty = true;
    cache_if_needed( );
  }

  bool BSICache::cache_if_needed( bool force ) {
    if ( nullptr != cache && ( values.size( ) >= DEFAULT_CACHE_LIMIT || force ) ) {
      std::fwrite( values.data( ), sizeof ( double ), values.size( ), cache );
      values.clear( );
      memrange.first = sizer;
      memrange.second = sizer;
      dirty = false;
    }

    return true;
  }

  void BSICache::uncache( size_t fromidx ) {
    if ( dirty ) {
      cache_if_needed( true );
    }

    const auto SIZE = sizeof (double );
    auto filepos = fromidx * SIZE;
    values.clear( );
    std::fseek( cache, filepos, SEEK_SET );
    auto reads = std::fread( values.data( ), SIZE, DEFAULT_CACHE_LIMIT, cache );
    memrange.first = fromidx;
    memrange.second = fromidx + reads;
  }

  double BSICache::value_at( size_t pos ) {
    if ( pos > sizer ) {
      return std::numeric_limits<double>::max( );
    }

    if ( pos < memrange.first || pos >= memrange.second ) {
      // time isn't in the memory cache, so we need to uncache it
      uncache( pos );
    }

    return values[pos - memrange.first];
  }

  void BSICache::fill( std::vector<double>& vec, size_t startidx, size_t stopidx ) {
    if ( stopidx >= sizer ) {
      stopidx = sizer;
    }
    const auto diff = stopidx - startidx;
    vec.reserve( vec.size( ) + diff );

    for (; startidx < stopidx; startidx++ ) {
      vec.push_back( value_at( startidx ) );
    }
  }

  void BSICache::name( const std::string& name ) {
    _name = name;
  }

  const std::string& BSICache::name( ) const {
    return _name;
  }

}