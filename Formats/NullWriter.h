/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NullWriter.h
 * Author: ryan
 *
 * Created on May 8, 2020, 7:02 AM
 */

#ifndef NULLWRITER_H
#define NULLWRITER_H

#include "Writer.h"

namespace FormatConverter {
  class SignalSet;
  
  class NullWriter : public Writer {
  public:
    NullWriter( );
    virtual ~NullWriter( );

  protected:
    virtual std::vector<std::string> closeDataSet( ) override;

    virtual int drain( SignalSet * info ) override;

  };
}

#endif /* NULLWRITER_H */

