/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TimeModifier.h"
#include "limits"

namespace FormatConverter{

  TimeModifier::TimeModifier( bool inited, dr_time desiredFirstTime ) : initialized( inited ),
  used( false ), _offset( desiredFirstTime ), _first( 0 ) {
  }

  TimeModifier::TimeModifier( const TimeModifier& model ) : initialized( model.initialized ),
  used( model.used ), _offset( model._offset ), _first( model._first ) {

  }

  TimeModifier::~TimeModifier( ) {
  }

  TimeModifier& TimeModifier::operator=(const TimeModifier& orig ) {
    if ( &orig != this ) {
      initialized = orig.initialized;
      _offset = orig._offset;
      _first = orig._first;
      used = orig.used;
    }
    return *this;
  }

  dr_time TimeModifier::offset( ) const {
    return _offset;
  }

  dr_time TimeModifier::firsttime( ) const {
    return _first;
  }

  dr_time TimeModifier::convert( const dr_time& orig ) {
    if ( !initialized ) {
      initialized = true;

      // if we haven't been initialized yet, then our offset is 
      // really our desired first time, so calculate what the real
      // offset should be
      _offset -= orig;
    }

    if ( !used ) {
      used = true;
      _first = orig;
    }

    return ( orig + _offset );
  }

  TimeModifier TimeModifier::passthru( ) {
    return offset( 0 );
  }

  TimeModifier TimeModifier::time( const dr_time& desiredStartTime ) {
    return TimeModifier( false, desiredStartTime );
  }

  TimeModifier TimeModifier::offset( const dr_time& offset ) {
    return TimeModifier( true, offset );
  }
}