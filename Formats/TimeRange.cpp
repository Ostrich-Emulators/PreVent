/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TimeRange.h"

namespace FormatConverter{

  TimeRange::TimeRange( size_t fromidx, size_t toidx, FILE * cachefile )
      : start( fromidx ), end( toidx ), cache( cachefile ), current( 0 ) { }

  TimeRange::~TimeRange( ) {
    fclose( cache );
  }

  TimeRange& TimeRange::operator++( ) {
    std::fseek( cache, sizeof (dr_time ), SEEK_CUR );
    return *this;
  }

  dr_time TimeRange::curr( ) const {
    return current;
  }

  dr_time TimeRange::next( ) {
    dr_time time;
    std::fread( &time, sizeof ( dr_time ), 1, cache );
    current = time;
    return time;
  }
}