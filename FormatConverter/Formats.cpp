/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Formats.h"

#include <string>

Formats::~Formats( ) {
}

Formats::Formats( ) {
}

Formats::Formats( const Formats& ) {
}

Format Formats::getValue( const std::string& fmt ) {
  if ( "wfdb" == fmt ) {
    return WFDB;
  }
  if ( "hdf5" == fmt ) {
    return HDF5;
  }
  if ( "stpxml" == fmt ) {
    return STPXML;
  }
  if ( "zl" == fmt ) {
    return DSZL;
  }
  return UNRECOGNIZED;
}
