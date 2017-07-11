/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WfdbWriter.h"
#include "ReadInfo.h"

WfdbWriter::WfdbWriter( ) {
}

WfdbWriter::WfdbWriter( const WfdbWriter& ) {
}

WfdbWriter::~WfdbWriter( ) {
}

void WfdbWriter::initDataSet( const std::string& newfile, int compression ) {
  
}

std::string WfdbWriter::closeDataSet( ) {

}

int WfdbWriter::drain( ReadInfo& ) {
  return 0;
}
