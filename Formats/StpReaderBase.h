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
#include <zlib.h>
#include <fstream>

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
    z_stream zipdata;

    void inflate_f( const std::string& input, const std::string& output );

    /**
     * Inflates the given input into the output.
     * @param input the data to be inflated
     * @param inputsize (in) the size of the input array; (out) the number
     * of bytes after the end of the stream (put back on the next read)
     * @param output the inflated data
     * @param outsize (in) the size of the inflated array; (out) the size of the
     * data (which will/should always be smaller)
     * @return one of zlib's Z_ constants
     */
    int inflate_b( unsigned char * input, size_t& inputsize,
        unsigned char * output, size_t& outsize );
    static std::string zerr( int Z);
  };
}
#endif /* STPBASE_H */

