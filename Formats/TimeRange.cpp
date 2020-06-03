/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TimeRange.h"
#include "SignalUtils.h"
#include <iostream>

namespace FormatConverter{

  TimeRange::TimeRangeIterator::TimeRangeIterator( TimeRange * o, size_t cnt ) : owner( o ), pos( cnt ) {
    std::fseek( owner->cache, pos * sizeof (dr_time ), SEEK_SET );
    std::fread( &curr, sizeof ( dr_time ), 1, owner->cache );
  }

  TimeRange::TimeRangeIterator& TimeRange::TimeRangeIterator::operator=(const TimeRangeIterator& orig ) {
    if ( this != &orig ) {
      this->owner = orig.owner;
      this->pos = orig.pos;
    }
    return *this;
  }

  TimeRange::TimeRangeIterator::~TimeRangeIterator( ) { }

  TimeRange::TimeRangeIterator& TimeRange::TimeRangeIterator::operator++( ) {
    pos++;
    std::fseek( owner->cache, pos * sizeof (dr_time ), SEEK_SET );
    std::fread( &curr, sizeof ( dr_time ), 1, owner->cache );
    return *this;
  }

  TimeRange::TimeRangeIterator& TimeRange::TimeRangeIterator::operator--( ) {
    pos--;
    if ( pos < 0 ) {
      pos = 0;
    }
    std::fseek( owner->cache, pos * sizeof (dr_time ), SEEK_SET );
    std::fread( &curr, sizeof ( dr_time ), 1, owner->cache );
    return *this;
  }

  bool TimeRange::TimeRangeIterator::operator==( TimeRangeIterator& other ) const {
    return ( this->owner == other.owner && this->pos == other.pos );
  }

  bool TimeRange::TimeRangeIterator::operator!=( TimeRangeIterator& other ) const {
    return !operator==( other );
  }

  dr_time TimeRange::TimeRangeIterator::operator*( ) {
    return curr;
  }

  TimeRange::TimeRange( ) : cache( SignalUtils::tmpf( ) ), sizer( 0 ) { }

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
    std::fwrite( vec.data( ), sizeof ( dr_time ), vec.size( ), cache );
    sizer += vec.size( );
  }

  void TimeRange::push_back( dr_time t ) {
    std::fwrite( &t, sizeof ( dr_time ), 1, cache );
    sizer++;
  }
}