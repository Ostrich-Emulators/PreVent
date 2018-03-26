/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DurationSpecification.h
 * Author: ryan
 *
 * Created on March 26, 2018, 12:04 PM
 */

#ifndef DURATIONSPECIFICATION_H
#define DURATIONSPECIFICATION_H

#include "dr_time.h"

class DurationSpecification {
public:

  virtual ~DurationSpecification( ) {
  }
  DurationSpecification( const dr_time& start, const dr_time& end );
  DurationSpecification( const DurationSpecification& );
  DurationSpecification& operator=(const DurationSpecification&);

  dr_time start( ) const;
  dr_time end( ) const;
  bool accepts( const dr_time& ) const;

  static DurationSpecification for_ms( const dr_time& start, long ms );
  static DurationSpecification for_s( const dr_time& start, long s );
  static DurationSpecification all();

private:
  DurationSpecification( ){}
  dr_time starttime;
  dr_time endtime;
};

#endif /* DURATIONSPECIFICATION_H */

