/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CacheFileHdf5Writer.h
 * Author: ryan
 *
 * Created on August 26, 2016, 12:55 PM
 */

#ifndef XMLREADER_H
#define XMLREADER_H

#include "Reader.h"
#include "DataRow.h"
#include "SignalSet.h"

#include <string>
#include <list>
#include <set>
#include <expat.h>
#include <fstream>

class SignalData;

class XmlReaderBase : public Reader {
public:
  static const std::set<std::string> Hz60;
  static const std::set<std::string> Hz120;
  static const int READCHUNK;

  XmlReaderBase( const std::string& name );
  virtual ~XmlReaderBase( );

protected:
  XmlReaderBase( const XmlReaderBase& orig );

  size_t getSize( const std::string& input ) const override;
  virtual int prepare( const std::string& input, SignalSet& info ) override;
  virtual ReadResult fill( SignalSet & info, const ReadResult& lastfill ) override;
  virtual void finish( ) override;

protected:
  /**
   * modify the given date. This is important when anonymizing data
   * @param 
   */
  time_t datemod( const time_t& rawdate ) const;
  /**
   * Trims the given string in-place, and returns it
   * @param totrim
   * @return
   */
  static std::string trim( std::string& totrim );

  void copysaved( SignalSet& newset );
  virtual void start( const std::string& element, std::map<std::string, std::string>& attrs ) = 0;
  virtual void end( const std::string& element, const std::string& text ) = 0;

  time_t time( const std::string& val ) const;
  bool isRollover( const time_t& now, const time_t& then ) const;
  void startSaving( );
  void setResult( ReadResult rslt );
  bool isFirstRead( ) const;
  void setDateModifier( const time_t& mod );

  SignalSet saved;
  SignalSet * filler;
private:

  static void start( void * data, const char * el, const char ** attr );
  static void end( void * data, const char * el );
  static void chars( void * data, const char * text, int len );
  static std::string working;
  static bool accumulateText;

  XML_Parser parser;
  std::ifstream input;
  ReadResult rslt;

  time_t datemodifier;
  bool firstread;
};

#endif /* XMLREADER_H */

