/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StpBase.h
 * Author: ryan
 *
 * Created on May 5, 2020, 8:34 AM
 */

#ifndef STPBASE_H
#define STPBASE_H

#include "Reader.h"
#include "CircularBuffer.h"
#include "zstr.hpp"

namespace FormatConverter {

  class StpReaderBase : public Reader {
  public:
    StpReaderBase( const std::string& name );
    StpReaderBase( const StpReaderBase& orig );
    virtual ~StpReaderBase( );

    virtual int prepare( const std::string& filename, std::unique_ptr<SignalSet>& data ) override;

    void setMetadataOnly( bool metasonly = true );
    bool isMetadataOnly( ) const;
    void finish( );

  protected:
    std::string popString( size_t length );
    int popInt16( );
    int popInt8( );
    unsigned int popUInt8( );
    unsigned int popUInt16( );
    unsigned int readUInt16( );
    unsigned long popUInt64( );

    int readMore( );

    CircularBuffer<unsigned char> work;
  private:
    bool metadataonly;
    zstr::ifstream * filestream;
  };
}
#endif /* STPBASE_H */

