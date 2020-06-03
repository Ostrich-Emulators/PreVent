/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TimeRange.h
 * Author: ryan
 *
 * Created on June 2, 2020, 8:54 AM
 */

#ifndef TIMERANGE_H
#define TIMERANGE_H

#include "dr_time.h"
#include <cstdio>
#include <iterator>
#include <vector>

namespace FormatConverter {

  class TimeRange {
  public:

    class TimeRangeIterator {
    public:
      using difference_type = dr_time;
      using value_type = dr_time;
      using pointer = const dr_time*;
      using reference = const dr_time&;
      using iterator_category = std::bidirectional_iterator_tag;

      TimeRangeIterator( TimeRange * owner, size_t pos );
      TimeRangeIterator& operator=(const TimeRangeIterator&);
      virtual ~TimeRangeIterator( );

      TimeRangeIterator& operator++( );
      TimeRangeIterator& operator--( );
      bool operator==( TimeRangeIterator& other ) const;
      bool operator!=( TimeRangeIterator& other ) const;
      dr_time operator*( );

    private:
      TimeRange * owner;
      dr_time curr;
      size_t pos;
    };

    TimeRange();
    virtual ~TimeRange( );

    TimeRangeIterator begin( );
    TimeRangeIterator end( );

    void push_back( const std::vector<dr_time>& vec );
    void push_back( dr_time t );

    size_t size( ) const;

  private:
    TimeRange& operator=(const TimeRange&);
    FILE * cache;
    size_t sizer;
  };
}
#endif /* TIMERANGE_H */

