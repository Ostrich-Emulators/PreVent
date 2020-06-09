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
    static const size_t CACHE_LIMIT;

    class TimeRangeIterator {
    public:
      using difference_type = dr_time;
      using value_type = dr_time;
      using pointer = const dr_time*;
      using reference = const dr_time&;
      //using iterator_category = std::bidirectional_iterator_tag;
      using iterator_category = std::forward_iterator_tag;

      TimeRangeIterator( TimeRange * owner, size_t pos );
      TimeRangeIterator& operator=(const TimeRangeIterator&);
      virtual ~TimeRangeIterator( );

      TimeRangeIterator& operator++( );
      TimeRangeIterator operator++( int );
      bool operator==( TimeRangeIterator& other ) const;
      bool operator!=( TimeRangeIterator& other ) const;
      dr_time operator*( );

    private:
      TimeRange * owner;
      size_t idx;
    };

    TimeRange( );
    virtual ~TimeRange( );

    TimeRangeIterator begin( );
    TimeRangeIterator end( );

    void push_back( const std::vector<dr_time>& vec );
    void push_back( dr_time t );

    size_t size( ) const;
    /**
     * Appends the values from this time range to the given vector. The vector
     * is not otherwise altered during this function
     * @param vec the vector to fill with values
     * @param startidx the first time index to add (inclusive)
     * @param stop the index to stop at (exclusive)
     */
    void fill( std::vector<dr_time>& vec, size_t startidx, size_t stopidx );

  private:
    dr_time time_at( size_t idx );
    bool cache_if_needed( bool force = false );
    void uncache( size_t fromidx );

    TimeRange& operator=(const TimeRange&);
    FILE * cache;
    size_t sizer;
    std::vector<dr_time> times;
    std::pair<size_t, size_t> memrange;
    bool dirty;
  };
}
#endif /* TIMERANGE_H */

