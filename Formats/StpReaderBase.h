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

    virtual int prepare( const std::string& filename, SignalSet * data ) override;

    virtual void setMetadataOnly( bool metasonly = true );
    virtual bool isMetadataOnly( ) const;
    virtual void finish( ) override;

    struct StpMetadata {
      std::string name;
      std::string mrn;
      dr_time start_utc;
      dr_time stop_utc;
      int segment_count;
    };

  protected:
    std::string popString( size_t length );
    std::string readString( size_t length );

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
    std::ifstream filestream;
    zstr::istream * zipstream;
    std::vector<size_t> segments;

    /**
     * Indexes the file, looking for compressed segments.
     * @param input the file to index
     * @return the compressed segment boundaries. If empty, the file is not compressed
     */
    std::vector<size_t> indexFile( const std::string& input );
    void inflate( const std::string& input, const std::string& output );
  };
}
#endif /* STPBASE_H */

