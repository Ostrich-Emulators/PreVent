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
#include "BasicSignalSet.h"

#include <string>
#include <list>
#include <set>
#include <expat.h>

class SignalData;
class StreamChunkReader;

class XmlReaderBase : public Reader {
public:
  static const std::set<std::string> Hz60;
  static const std::set<std::string> Hz120;
  static const int READCHUNK;

  XmlReaderBase( const std::string& name );
  virtual ~XmlReaderBase( );

  virtual int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
  virtual ReadResult fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastfill = ReadResult::FIRST_READ ) override;
  virtual void finish( ) override;

protected:
  XmlReaderBase( const XmlReaderBase& orig );

protected:
  /**
   * Trims the given string in-place, and returns it
   * @param totrim
   * @return
   */
  static std::string trim( std::string& totrim );

  void copySavedInto( std::unique_ptr<SignalSet>& newset );
  virtual void start( const std::string& element, std::map<std::string, std::string>& attrs ) = 0;
  virtual void end( const std::string& element, const std::string& text ) = 0;
  virtual void comment( const std::string& text ) = 0;

  /**
   * Converts the given time string to a UTC time
   * @param val
   * @param valIsLocal is the string given in val localtime or already in UTC?
   * @return UTC time
   */
  dr_time time( const std::string& val, bool valIsLocal = false ) const;
  bool isRollover( const dr_time& now, const dr_time& then ) const;
  void startSaving( dr_time now );
  void setResult( ReadResult rslt );

  BasicSignalSet saved;
  SignalSet * filler;
private:

  static void start( void * data, const char * el, const char ** attr );
  static void end( void * data, const char * el );
  static void chars( void * data, const char * text, int len );
  static void comment( void * data, const char * text );
  static std::string working;
  static bool accumulateText;

  XML_Parser parser;
  std::unique_ptr<StreamChunkReader> input;
  ReadResult rslt;
  long tz_offset;
  std::string tz_name;

  dr_time lastSaveTime;
};

#endif /* XMLREADER_H */

