/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WfdbWriter.h
 * Author: ryan
 *
 * Created on July 11, 2017, 7:16 AM
 */

#ifndef WFDBWRITER_H
#define WFDBWRITER_H

#include "Writer.h"

#include <map>
#include <memory>
#include <wfdb/wfdb.h>

namespace FormatConverter {
  class SignalData;

  class WfdbWriter : public Writer {
  public:
    WfdbWriter( );
    virtual ~WfdbWriter( );

  protected:
    int initDataSet( );
    std::vector<std::string> closeDataSet( );
    int drain( SignalSet * );

  private:
    WfdbWriter( const WfdbWriter& orig );

    int write( double freq, std::vector<SignalData *> data, const std::string& filestart );
    void syncAndWrite( double freq, std::vector<SignalData *> data );

    std::string fileloc;
    std::string currdir;
    std::vector<std::string> files;
    std::map<std::string, WFDB_Siginfo> sigmap;
  };
}
#endif /* WFDBWRITER_H */

