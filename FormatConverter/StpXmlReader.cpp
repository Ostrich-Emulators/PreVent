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
#include <libxml/parser.h>
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

void startDoc( void * user_data ){
  ((StpXmlReader *)user_data )->start();
}

void endDoc( void * user_data ){
  ((StpXmlReader *)user_data )->end();
}

int StpXmlReader::prepare( const std::string& input, ReadInfo& info ) {
  xmlSAXHandler handler;
  handler.internalSubset = NULL;
  handler.isStandalone = NULL;
  handler.hasInternalSubset = NULL;
  handler.hasExternalSubset = NULL;
  handler.resolveEntity = NULL;
  handler.getEntity = NULL;
  handler.entityDecl = NULL;
  handler.notationDecl = NULL;
  handler.attributeDecl = NULL;
  handler.elementDecl = NULL;
  handler.unparsedEntityDecl = NULL;
  handler.setDocumentLocator = NULL;
  handler.startDocument = &startDoc;
  handler.endDocument = &endDoc;
  handler.startElement = NULL;
  handler.endElement = NULL;
  handler.reference = NULL;
  handler.characters = NULL;
  handler.ignorableWhitespace = NULL;
  handler.processingInstruction = NULL;
  handler.comment = NULL;
  handler.warning = NULL;
  handler.error = NULL;
  handler.fatalError = NULL;


  

  if ( xmlSAXUserParseFile( &handler, this, "ZUMPO_736E-1459787794.xml" ) < 0 ) {
    std::cerr << "got here?" << std::endl;
    return 0;
  }
  std::cout << "end of function" << std::endl;
}

ReadResult StpXmlReader::readChunk( ReadInfo & info ) {
  return ReadResult::ERROR;
}

void StpXmlReader::start() {
  std::cout << "into start doc" << std::endl;
}

void StpXmlReader::end() {
  std::cout << "into end doc" << std::endl;
}