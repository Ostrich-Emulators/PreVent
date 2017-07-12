/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CacheFileHdf5Writer.cpp
 * Author: ryan
 * 
 * Created on August 26, 2016, 12:55 PM
 * 
 * Almost all the zlib code was taken from http://www.zlib.net/zlib_how.html
 */

#include "StpXmlReader.h"
#include "SignalData.h"
#include "DataRow.h"
#include "Hdf5Writer.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>

StpXmlReader::StpXmlReader( ) {
}

StpXmlReader::StpXmlReader( const StpXmlReader& orig ) {
}

StpXmlReader::~StpXmlReader( ) {
}

void StpXmlReader::finish( ) {
}

int StpXmlReader::getSize( const std::string& input ) const {
  struct stat info;

  if ( stat( input.c_str( ), &info ) < 0 ) {
    perror( input.c_str( ) );
    return -1;
  }

  return info.st_size;
}

int StpXmlReader::prepare( const std::string& input, ReadInfo& ) {
  return 0;
}

ReadResult StpXmlReader::readChunk( ReadInfo& info ) {
  return ReadResult::ERROR;
}
