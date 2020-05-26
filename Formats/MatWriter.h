/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MatWriter.h
 * Author: ryan
 *
 * Created on July 11, 2017, 7:16 AM
 */

#ifndef MATWRITER_H
#define MATWRITER_H

#include "Writer.h"
#include <iostream>
#include <fstream>

#include <ctime>
#include <matio.h>
#include <memory>

namespace FormatConverter {
  class SignalData;
  class SignalSet;

  enum MatVersion {
    MV4, MV5, MV7
  };

  class MatWriter : public Writer {
  public:
    MatWriter( MatVersion version = MV5 );
    virtual ~MatWriter( );

  protected:
    int initDataSet( );
    std::vector<std::string> closeDataSet( );
    int drain( SignalSet * );

  private:
    MatWriter( const MatWriter& orig );

    /**
     * Consumes the given data and writes to the file
     * @param data
     * @return
     */
    int writeVitals( std::vector<SignalData *> data );
    int writeWaves( double freq, std::vector<SignalData *> data );

    int writeStrings( const std::string& label, std::vector<std::string>& strings );

    std::string tempfileloc;
    mat_t * matfile;
    SignalSet * dataptr;
    matio_compression compress;
    MatVersion version;
  };
}

#endif /* MATWRITER_H */
