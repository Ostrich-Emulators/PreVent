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

class SignalData;
class StreamChunkReader;

/**
 * A reader for the HSDI signal files that UVA is producing
 */
class ZlReader2 : public Reader {
public:
  ZlReader2( );
  ZlReader2( const std::string& name );
  virtual ~ZlReader2( );

  static bool waveIsOk( const std::string& wavedata );
protected:
  ReadResult fill( std::unique_ptr<SignalSet>&, const ReadResult& lastfill ) override;
  int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
  void finish( ) override;

private:
  ZlReader2( const ZlReader2& orig );

  bool firstread;
  dr_time currentTime;
  std::map<std::string, std::unique_ptr<StreamChunkReader>> signalToReaderLkp;
  std::map<std::string, std::string> leftovers;

  /**
   * Returns the current text from the input file, in nice JSON-ready chunks.
   * Each chunk will be some number of 3-index-big arrays of strings
   * (time (ms), sequence #, data points)
   * @param signalname
   * @param text
   * @return
   */
  std::string normalizeText( const std::string& signalname, std::string text );
};

#endif /* ZlReader2_H */

