/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dr_time.h
 * Author: ryan
 *
 * Created on February 7, 2018, 9:55 AM
 */

#ifndef DR_TIME_H
#define DR_TIME_H

#include "tz.h"
#include <iostream>
namespace FormatConverter {

  class dr_time : public date::zoned_time<std::chrono::milliseconds> {
  public:
    const static dr_time localzero;
    const static dr_time gmtzero;

    static dr_time fromLocal( const time_t& t = 0 );
    static dr_time fromGmt( const time_t& t = 0 );
    static dr_time now();
  };

  bool operator<(const dr_time& one, const dr_time& two );
  bool operator>(const dr_time& one, const dr_time& two );
}

#endif /* DR_TIME_H */

