/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NullReader.h
 * Author: ryan
 *
 * Created on July 7, 2017, 2:57 PM
 */

#ifndef NULLREADER_H
#define NULLREADER_H

#include "Reader.h"

#include <map>
#include <set>

class NullReader : public Reader {
public:
  NullReader( const std::string& name );
  virtual ~NullReader( );

  int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
  ReadResult fill( std::unique_ptr<SignalSet>& data,
      const ReadResult& lastresult = ReadResult::FIRST_READ ) override;
};

#endif /* NULLREADER_H */

