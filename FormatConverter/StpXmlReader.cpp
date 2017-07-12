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

#include <iostream>
#include <sys/stat.h>

typedef void (StpXmlReader::*VoidFnc )(void *);

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

int StpXmlReader::prepare( const std::string& input, ReadInfo& info ) {
  xmlSAXHandler handler = { NULL };
  handler.startDocument = &StpXmlReader::start;
  handler.endDocument = &StpXmlReader::finish;
  handler.startElement = &StpXmlReader::startElement;
  handler.endElement = &StpXmlReader::endElement;
  handler.characters = &StpXmlReader::chars;

  if ( xmlSAXUserParseFile( &handler, this, "ZUMPO_736E-1459787794.xml" ) < 0 ) {
    std::cerr << "got here?" << std::endl;
    return 0;
  }
  std::cout << "end of function" << std::endl;
}

ReadResult StpXmlReader::readChunk( ReadInfo & info ) {
  return ReadResult::ERROR;
}

void StpXmlReader::start( void * ) {
  std::cout << "into start do2c" << std::endl;
}

void StpXmlReader::finish( void * ) {
  std::cout << "into end do2c" << std::endl;
}

void StpXmlReader::chars( void * user_data, const xmlChar * ch, int len ) {
  std::cout << "->" << std::string( (char *) ch, len ) << "<-" << std::endl;
  ( (StpXmlReader *) user_data )->append( std::string( (char *) ch, len ) );
}

void StpXmlReader::append( const std::string& s ) {
  leftoverText += s;
  std::cout << "working text: " << leftoverText << std::endl;
}

void StpXmlReader::startElement( void * user_data, const xmlChar * name, const xmlChar ** attrs ) {
  std::map<std::string, std::string> attrmap;
  int idx = 0;
  if ( NULL != attrs ) {
    char * key = (char *) attrs[idx];
    while ( NULL != key ) {
      std::string val( (char *) attrs[++idx] );
      attrmap[std::string( key )] = val;
      key = (char *) attrs[++idx];
    }
  }

  ( (StpXmlReader *) user_data )->setElement( std::string( (char *) name ), attrmap );
}

void StpXmlReader::setElement( const std::string& name, std::map<std::string, std::string>& attrs ) {
  std::cout << "start " << name << std::endl;
  element = name;
  for ( auto m : attrs ) {
    std::cout << "  " << m.first << ": " << m.second << std::endl;
  }
}

void StpXmlReader::endElement( void * user_data, const xmlChar * name ) {
  std::cout << "end " << name << std::endl;
  ( (StpXmlReader *) user_data )->reset( );
}

void StpXmlReader::reset( ) {
  std::cout << "  cleared text " << std::endl;
  leftoverText.clear( );
}
