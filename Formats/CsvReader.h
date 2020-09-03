/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CsvReader.h
 * Author: ryan
 *
 * Created on September 1, 2020, 9:23 PM
 */

#ifndef CSVREADER_H
#define CSVREADER_H

#include "Reader.h"

#include <vector>
#include <string>
#include <fstream>
#include "dr_time.h"

namespace FormatConverter {

  class CsvReader : public Reader {
  public:
    CsvReader( );

    virtual ~CsvReader( );

  protected:
    int prepare( const std::string& input, SignalSet * info ) override;
    ReadResult fill( SignalSet * data, const ReadResult& lastfill ) override;

  private:
    std::vector<std::string> linevalues( const std::string& csvline, dr_time & timer );
    dr_time converttime( const std::string& timeline );
    void loadMetas( SignalSet * info );

    std::ifstream datafile;
    std::vector<std::string> metadata;
    std::vector<std::string> headings;
    dr_time lasttime;
  };
}

#endif /* CSVREADER_H */

