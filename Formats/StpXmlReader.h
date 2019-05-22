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

#ifndef STPXMLREADER_H
#define STPXMLREADER_H

#include "XmlReaderBase.h"
#include <string>
#include <list>
#include <set>

class SignalData;

class StpXmlReader : public XmlReaderBase {
public:
  static const std::set<std::string> Hz60;
  static const std::set<std::string> Hz120;
  static const int INHEADER;
  static const int INVITAL;
  static const int INWAVE;
  static const int INNAME;
  static const int INDETERMINATE;

  StpXmlReader( );
  virtual ~StpXmlReader( );

private:
  StpXmlReader( const StpXmlReader& orig );

  static std::string resample( const std::string& data, int hz );
  static bool waveIsOk( const std::string& wavedata );

  virtual void start( const std::string& element, std::map<std::string, std::string>& attrs ) override;
  virtual void end( const std::string& element, const std::string& text ) override;
  virtual void comment( const std::string& text ) override;

  void setstate( int state );

  time_t prevtime;
  time_t currvstime;
  time_t lastvstime;
  time_t currwavetime;
  time_t lastwavetime;
  time_t recordtime;
  long currsegidx;
  bool warnMissingName;
  bool warnJunkData;
  bool v8;
  bool isphilips;
  bool isix;
  bool warnedix;
  bool skipwave;
  bool skipvital;

  int state;
  std::string label;
  std::string value;
  std::string uom;
  int v8samplerate;
  std::map<std::string, std::string> attrs;
};

#endif /* STPXMLREADER_H */
