/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <algorithm>

#include "DurationSpecification.h"

DurationSpecification::DurationSpecification( const dr_time& start, const dr_time& end ) :
starttime( start ), endtime( end ) {
}

DurationSpecification::DurationSpecification( const DurationSpecification& o ) :
starttime( o.starttime ), endtime( o.endtime ) {
}

DurationSpecification& DurationSpecification::operator=(const DurationSpecification& o ) {
  if ( this != &o ) {
    starttime = o.starttime;
    endtime = o.endtime;
  }
  return *this;
}

dr_time DurationSpecification::start( ) const {
  return starttime;
}

dr_time DurationSpecification::end( ) const {
  return endtime;
}

bool DurationSpecification::accepts( const dr_time& t ) const {
  return ( t < endtime && t >= starttime );
}

DurationSpecification DurationSpecification::for_ms( const dr_time& start, long ms ) {
  return DurationSpecification( start, start + ms );
}

DurationSpecification DurationSpecification::for_s( const dr_time& start, long s ) {
  return for_ms( start, s * 1000 );
}

DurationSpecification DurationSpecification::all() {
  return DurationSpecification( 0, std::numeric_limits<dr_time>::max() );
}
