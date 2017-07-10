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

#ifndef CACHEFILEHDF5WRITER_H
#define CACHEFILEHDF5WRITER_H

#include <map>
#include <string>
#include <memory>
#include <istream>

#include "FromReader.h"

class SignalData;


enum zlReaderState { IN_HEADER, IN_VITAL, IN_WAVE, IN_TIME };
class ZlReader {
public:
  ZlReader( const std::string& outputdir, int compression, bool bigfile,
			const std::string& prefix );
  virtual ~ZlReader( );

  int convert( const std::string& input );
  int convert( std::istream&, bool compressed=false );

protected:
	int readChunk( const std::string& input );
	int getSize( const std::string& input );

private:
  static const int CHUNKSIZE;

  ZlReader( const ZlReader& orig );

  const std::string outputdir;
  const bool largefile;
  const std::string prefix;
  bool firstheader;
  std::string workingText;
  int compression;
  time_t currentTime;
  time_t firstTime;
  time_t lastTime;
  int ordinal;
  zlReaderState state;

  std::map<std::string, std::string> datasetattrs;
  std::map<std::string, std::unique_ptr<SignalData>> vitals;
  std::map<std::string, std::unique_ptr<SignalData>> waves;
  
  void handleInputChunk( std::string& chunk );
  void handleOneLine( const std::string& chunk );
  void reset();
  void flush();
  int convertcompressed( std::istream& );
  
  static const std::string HEADER;
  static const std::string VITAL;
  static const std::string WAVE;
  static const std::string TIME;  
};

#endif /* CACHEFILEHDF5WRITER_H */

