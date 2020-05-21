/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "NullWriter.h"

namespace FormatConverter{

  NullWriter::NullWriter( ) : Writer( "noop" ) { }

  NullWriter::~NullWriter( ) { }

  std::vector<std::string> NullWriter::closeDataSet( ) {
    return std::vector<std::string>( );
  }

  int NullWriter::drain( SignalSet * info ) {
    return 0;
  }
}