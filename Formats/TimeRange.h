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

namespace FormatConverter {

  class TimeRange {
  public:
    TimeRange( size_t fromidx, size_t toidx, FILE * cachefile );
    TimeRange& operator++( );
    virtual ~TimeRange( );

    size_t size( ) const;
    dr_time curr( ) const;
    dr_time next( );

  private:
    const size_t start;
    const size_t end;
    FILE * cache;
    dr_time current;
  };
}
#endif /* TIMERANGE_H */

