/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WfdbReader.h
 * Author: ryan
 *
 * Created on July 7, 2017, 2:57 PM
 */

#ifndef TDMSREADER_H
#define TDMSREADER_H

#include "Reader.h"
#include <memory>
#include <string>
#include <TdmsParser.h>

class SignalData;

class TdmsReader : public Reader {
public:
  TdmsReader( );
  virtual ~TdmsReader( );

protected:
  int prepare( const std::string& input, SignalSet& info ) override;
  void finish( ) override;

  ReadResult fill( SignalSet& data, const ReadResult& lastfill ) override;
  size_t getSize( const std::string& input ) const override;

private:
  std::unique_ptr<TdmsParser> parser;

  static dr_time parsetime( const std::string& timestr );
  bool writeWaveChunkAndReset( int& count, int& nancount, std::vector<double>& doubles,
        bool& seenFloat, std::unique_ptr<SignalData>& signal, dr_time& time, int timeinc );
};

#endif /* WFDBREADER_H */

