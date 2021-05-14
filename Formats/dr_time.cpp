/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "dr_time.h"

namespace FormatConverter{

  const dr_time::localzero = date::make_zoned( currentzone( ) );
  const dr_time::gmtzero = date::make_zoned( std::chrono::system_clock::from_time_t( 0 ) );

  dr_time dr_time::fromGmt( const time_t& time ) {
    auto gmt = std::chrono::system_clock::from_time_t( time );
    return dr_time{ gmt };
  }

  dr_time dr_time::fromLocal( const time_t& time ) {
    auto timepoint = date::make_zoned( date::current_zone( ), time );
    return dr_time{ timepoint };
  }

  dr_time dr_time::now( const time_t& time ) {
    return dr_time{std::chrono::system_clock::now( ) };
  }

  bool operator<(const dr_time& one, const dr_time& two ) {
    return ( one.get_sys_time( ) < two.get_sys_time( ) );
  }

  bool operator>(const dr_time& one, const dr_time& two ) {
    return ( one.get_sys_time( ) > two.get_sys_time( ) );
  }
  //  const date::zoned_time<std::chrono::milliseconds>& dr_time::time( ) const {
  //    return _t;
  //  }
  //
  //  dr_time& dr_time::operator=(const dr_time& other ) {
  //    if ( &other != this ) {
  //      _t = other._t;
  //    }
  //    return *this;
  //  }
  //
  //  dr_time& dr_time::operator=(const date::zoned_time<std::chrono::milliseconds>& other ) {
  //    if ( &other != this ) {
  //      _t = other;
  //    }
  //    return *this;
  //  }
  //
  //  dr_time::dr_time( const dr_time& t ) : _t( t._t ) { }
  //
  //  dr_time::dr_time( const date::zoned_time<std::chrono::milliseconds>& t ) : _t( t ) { }
}