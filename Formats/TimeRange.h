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

#include "FileCachingVector.h"

namespace FormatConverter {

  class TimeRange : public FileCachingVector<dr_time> {
  };
}
#endif /* TIMERANGE_H */

