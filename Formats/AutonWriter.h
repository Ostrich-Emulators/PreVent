/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AutonWriter.h
 * Author: ryan
 *
 * Created on June 28, 2020, 1:54 PM
 */

#ifndef AUTONWRITER_H
#define AUTONWRITER_H

#include <H5Cpp.h>
#include <H5Cpp.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <memory>
#include <ctime>
#include "Writer.h"
#include "dr_time.h"

namespace FormatConverter {
  class SignalSet;
  class SignalData;
  class TimeRange;

  class AutonWriter : public Writer {
  public:
    AutonWriter( );
    virtual ~AutonWriter( );

  protected:
    std::vector<std::string> closeDataSet( );
    int drain( SignalSet * );

  private:
    void writeGlobalMetas( H5::H5File& file );
    void writeVital( H5::H5Object& loc, SignalData * );
    void writeWave( H5::H5Object& loc, SignalData * );
    void writeMetas( H5::H5Object& loc, SignalData * );

    SignalSet * dataptr;

    struct autondata {
      double time;
      double value;
    };
  };
}
#endif /* AUTONWRITER_H */

