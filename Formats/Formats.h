/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Formats.h
 * Author: ryan
 *
 * Created on July 7, 2017, 2:24 PM
 */

#ifndef FORMATS_H
#define FORMATS_H

#include <string>

// valid formats
namespace FormatConverter {

  enum Format {
    UNRECOGNIZED, NOOP, WFDB, HDF5, STPXML, DSZL, MAT5, MAT4, MAT73,
    CSV, CPCXML, STPJSON, MEDI, STPGE, STPP, DWC
  };

  class Formats {
  public:
    static Format getValue( const std::string& fmt );
    static Format guess( const std::string& filename );
    virtual ~Formats( );
  private:
    Formats( );
    Formats( const Formats& );
  };
}
#endif /* FORMATS_H */

