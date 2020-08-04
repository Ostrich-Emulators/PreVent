/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Formats.h"
#include "Log.h"

#include <algorithm>
#include <string>
#include <iostream>

namespace FormatConverter{

  Formats::~Formats( ) { }

  Formats::Formats( ) { }

  Formats::Formats( const Formats& ) { }

  Format Formats::getValue( const std::string& fmt ) {
    if ( "wfdb" == fmt ) {
      return WFDB;
    }
    if ( "hdf5" == fmt ) {
      return HDF5;
    }
    if ( "stpge" == fmt ) {
      return STPGE;
    }
    if ( "stpp" == fmt ) {
      return STPP;
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
      return MEDI;
    }
    if ( "dwc" == fmt ) {
      return DWC;
    }
    if ( "noop" == fmt ) {
      return NOOP;
    }

    return UNRECOGNIZED;
  }

  Format Formats::guess( const std::string& filename ) {
    if ( "noop" == filename ) {
      return NOOP;
    }

    int idx = filename.find_last_of( '.' );
    if ( idx > 0 ) {
      std::string suffix = filename.substr( idx + 1 );
      std::transform( suffix.begin( ), suffix.end( ), suffix.begin( ), ::tolower );

      if ( "mat" == suffix ) {
        return Format::MAT5;
      }
      else if ( "hdf5" == suffix || "h5" == suffix ) {
        return Format::HDF5;
      }
      else if ( "xml" == suffix ) {
        return Format::STPXML;
      }
      else if ( "zl" == suffix || "gzip" == suffix || "gz" == suffix ) {
        return Format::DSZL;
      }
      else if ( "hea" == suffix ) {
        return Format::WFDB;
      }
      else if ( "stpge" == suffix ) {
        return Format::STPGE;
      }
      else if ( "stpp" == suffix ) {
        return Format::STPP;
      }
      else if ( "stp" == suffix ) {
        Log::info() << "\"stp\" interpreted as \"stpge\", use \"stpp\" for Philips version" << std::endl;
        return Format::STPGE;
      }
      else if ( "csv" == suffix ) {
        return Format::CSV;
      }
      else if ( "json" == suffix ) {
        return Format::STPJSON;
      }
      else if ( "tdms" == suffix || "medi" == suffix ) {
        return Format::MEDI;
      }
      else if ( "info" == suffix ) {
        return Format::DWC;
      }
    }
    return Format::UNRECOGNIZED;
  }
}