/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ZlReader2.h
 * Author: ryan
 *
 */

#ifndef ZlReader2_H
#define ZlReader2_H

#include "Reader.h"
#include "StreamChunkReader.h"
#include <map>
#include <string>
#include <memory>
#include <istream>
#include <zlib.h>

#include "json.hpp"

class SignalData;
class StreamChunkReader;

/**
 * A sax-like parser for JSON files
 */
class SaxJson : public nlohmann::json_sax<nlohmann::json> {
public:
  SaxJson( std::unique_ptr<SignalData>& signl );
  bool null( );

  // called when a boolean is parsed; value is passed
  bool boolean( bool val );

  // called when a signed or unsigned integer number is parsed; value is passed
  bool number_integer( number_integer_t val );
  bool number_unsigned( number_unsigned_t val );

  // called when a floating-point number is parsed; value and original string is passed
  bool number_float( number_float_t val, const string_t& s );

  // called when a string is parsed; value is passed and can be safely moved away
  bool string( string_t& val );

  // called when an object or array begins or ends, resp. The number of elements is passed (or -1 if not known)
  bool start_object( std::size_t elements );
  bool end_object( );
  bool start_array( std::size_t elements );
  bool end_array( );
  // called when an object key is parsed; value is passed and can be safely moved away
  bool key( string_t& val );

  // called when a parse error occurs; byte position, the last token, and an exception is passed
  bool parse_error( std::size_t position, const std::string& last_token, const nlohmann::detail::exception& ex );
private:
  std::unique_ptr<SignalData>& signal;
  std::vector<std::string> data;
};

/**
 * A reader for the HSDI signal files that UVA is producing
 */
class ZlReader2 : public Reader {
public:
  ZlReader2( );
  ZlReader2( const std::string& name );
  virtual ~ZlReader2( );

protected:
  ReadResult fill( std::unique_ptr<SignalSet>&, const ReadResult& lastfill ) override;
  size_t getSize( const std::string& input ) const override;

  int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
  void finish( ) override;

private:
  ZlReader2( const ZlReader2& orig );

  bool firstread;
  dr_time currentTime;
  std::map<std::string, std::unique_ptr<StreamChunkReader>> signalToReaderLkp;
  std::map<std::string, std::unique_ptr<SaxJson>> signalToParserLkp;
  std::map<std::string, std::string> leftovers;

  std::string normalizeText( const std::string& signalname, std::string text );
};

#endif /* ZlReader2_H */

