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

#ifndef ZLREADER_H
#define ZLREADER_H

#include "Reader.h"
#include "StreamChunkReader.h"
#include <map>
#include <string>
#include <memory>
#include <istream>
#include <zlib.h>

class SignalData;
class StreamChunkReader;

enum zlReaderState {
  ZIN_HEADER, ZIN_VITAL, ZIN_WAVE, ZIN_TIME
};

class ZlReader : public Reader {
public:
  ZlReader( );
  ZlReader( const std::string& name );
  virtual ~ZlReader( );

protected:
  ReadResult fill( std::unique_ptr<SignalSet>&, const ReadResult& lastfill ) override;

  int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
  void finish( ) override;

private:

  ZlReader( const ZlReader& orig );

  bool firstread;
  std::string leftoverText;
  dr_time currentTime;
  zlReaderState state;
  std::unique_ptr<StreamChunkReader> stream;

  void handleInputChunk( std::string& chunk, std::unique_ptr<SignalSet>& info );
  void handleOneLine( const std::string& chunk, std::unique_ptr<SignalSet>& info );

  static const std::string HEADER;
  static const std::string VITAL;
  static const std::string WAVE;
  static const std::string TIME;
};

#endif /* ZLREADER_H */

