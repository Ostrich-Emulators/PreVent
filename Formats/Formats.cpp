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
  if ( "zl" == fmt || "dszl" == fmt ) {
    return DSZL;
  }
  if ( "mat73" == fmt || "matlab73" == fmt ) {
    return MAT73;
  }
  if ( "mat4" == fmt || "matlab4" == fmt ) {
    return MAT4;
  }
  if ( "mat" == fmt || "matlab" == fmt || "mat5" == fmt || "matlab5" == fmt ) {
    return MAT5;
  }
  if ( "csv" == fmt ) {
    return CSV;
  }
  if ( "cpcxml" == fmt ) {
    return CPCXML;
  }
  if ( "tdms" == fmt || "medi" == fmt ) {
    return TDMS;
  }

  return UNRECOGNIZED;
}

Format Formats::guess( const std::string& filename ) {
  int idx = filename.find_last_of( '.' );
  if ( idx > 0 ) {
    std::string suffix = filename.substr( idx + 1 );
    if ( "mat" == suffix ) {
      return Format::MAT5;
    }
    else if ( "hdf5" == suffix || "h5" == suffix ) {
      return Format::HDF5;
    }
    else if ( "xml" == suffix ) {
      return Format::STPXML;
    }
    else if ( "zl" == suffix ) {
      return Format::DSZL;
    }
    else if ( "hea" == suffix ) {
      return Format::WFDB;
    }
    else if ( "csv" == suffix ) {
      return Format::CSV;
    }
    else if ( "json" == suffix ) {
      return Format::STPJSON;
    }
    else if ( "tdms" == suffix || "medi" == suffix ) {
      return Format::TDMS;
    }
  }
  return Format::UNRECOGNIZED;
}
