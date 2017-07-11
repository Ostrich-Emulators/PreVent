/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WfdbWriter.h"
#include "ReadInfo.h"

#include <wfdb/wfdb.h>

WfdbWriter::WfdbWriter( ) {
}

WfdbWriter::WfdbWriter( const WfdbWriter& ) {
}

WfdbWriter::~WfdbWriter( ) {
}

int WfdbWriter::initDataSet( const std::string& newfile, int compression ) {
  return newheader( newfile.c_str( ) ); // check that our header file is valid
}

std::string WfdbWriter::closeDataSet( ) {

}

int WfdbWriter::drain( ReadInfo& info ) {
  



  return 0;
}
