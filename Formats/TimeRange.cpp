/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TimeRange.h"
#include "SignalUtils.h"
#include <iostream>

namespace FormatConverter{
  const size_t TimeRange::CACHE_LIMIT = 1024 * 384; // totally arbitrary, about 3MB data

  TimeRange::TimeRangeIterator::TimeRangeIterator( TimeRange * o, size_t cnt )
      : owner( o ), idx( cnt ) { }

  TimeRange::TimeRangeIterator& TimeRange::TimeRangeIterator::operator=(const TimeRangeIterator& orig ) {
    if ( this != &orig ) {
      this->owner = orig.owner;
      this->idx = orig.idx;
    }
    return *this;
  }

  TimeRange::TimeRangeIterator::~TimeRangeIterator( ) { }

  TimeRange::TimeRangeIterator& TimeRange::TimeRangeIterator::operator++( ) {
    idx++;
    return *this;
  }

  bool TimeRange::TimeRangeIterator::operator==( TimeRangeIterator& other ) const {
    return ( this->owner == other.owner && this->idx == other.idx );
  }

  bool TimeRange::TimeRangeIterator::operator!=( TimeRangeIterator& other ) const {
    return !operator==( other );
  }

  dr_time TimeRange::TimeRangeIterator::operator*( ) {
    return owner->time_at( idx );
  }

  TimeRange::TimeRange( ) : cache( SignalUtils::tmpf( ) ), sizer( 0 ),
      memrange( 0, 0 ), dirty( false ) { }

  TimeRange::~TimeRange( ) {
    std::fclose( cache );
  }

  size_t TimeRange::size( ) const {
    return sizer;
  }

  TimeRange::TimeRangeIterator TimeRange::begin( ) {
    return TimeRangeIterator( this, 0 );
  }

  TimeRange::TimeRangeIterator TimeRange::end( ) {
    return TimeRangeIterator( this, sizer + 1 );
  }

  void TimeRange::push_back( const std::vector<dr_time>& vec ) {
    times.insert( times.end( ), vec.begin( ), vec.end( ) );
    sizer += vec.size( );
    memrange.second += vec.size( );
    dirty = true;
    cache_if_needed( );
  }

  void TimeRange::push_back( dr_time t ) {
    times.push_back( t );
    sizer++;
    memrange.second++;
    dirty = true;
    cache_if_needed( );
  }

  bool TimeRange::cache_if_needed( bool force ) {
    if ( times.size( ) >= CACHE_LIMIT || force ) {
      std::fwrite( times.data( ), sizeof ( dr_time ), times.size( ), cache );
      times.clear( );
      memrange.first = sizer;
      memrange.second = sizer;
      dirty = false;
    }

    return true;
  }

  void TimeRange::uncache( size_t fromidx ) {
    if ( dirty ) {
      cache_if_needed( true );
    }

    const auto SIZE = sizeof (dr_time );
    auto filepos = fromidx * SIZE;
    times.clear( );
    std::fseek( cache, filepos, SEEK_SET );
    auto reads = std::fread( times.data( ), SIZE, CACHE_LIMIT, cache );
    memrange.first = fromidx;
    memrange.second = fromidx + reads;
  }

  dr_time TimeRange::time_at( size_t pos ) {
    if ( pos > sizer ) {
      return std::numeric_limits<dr_time>::max( );
    }

    if ( pos < memrange.first || pos >= memrange.second ) {
      // time isn't in the memory cache, so we need to uncache it
      uncache( pos );
    }

    return times[pos - memrange.first];
  }

  void TimeRange::fill( std::vector<dr_time>& vec, size_t startidx, size_t stopidx ) {
    if ( stopidx >= sizer ) {
      stopidx = sizer;
    }
    const auto diff = stopidx - startidx;
    vec.reserve( vec.size( ) + diff );

    for (; startidx < stopidx; startidx++ ) {
      vec.push_back( time_at( startidx ) );
    }
  }
}