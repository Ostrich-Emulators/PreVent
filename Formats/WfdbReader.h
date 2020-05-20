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

#ifndef WFDBREADER_H
#define WFDBREADER_H

#include "Reader.h"
#include <ctime>
#include <wfdb/wfdb.h>

namespace FormatConverter {

  class WfdbReader : public Reader {
  public:
    WfdbReader( );

    virtual ~WfdbReader( );

    dr_time convert( const char * timestr );
    dr_time basetime( ) const;

  protected:
    WfdbReader( const std::string& name );
    virtual int prepare( const std::string& input, SignalSet * info ) override;
    virtual void finish( ) override;
    void setBaseTime( const dr_time& time );
    int base_ms() const;

    virtual ReadResult fill( SignalSet * data, const ReadResult& lastfill ) override;

  protected:
    int sigcount;
    WFDB_Siginfo * siginfo;
    int interval;
    unsigned int freqhz;
    dr_time curtime;
    dr_time _basetime;
    int extra_ms;
    bool basetimeset;
    size_t framecount;
  };
}
#endif /* WFDBREADER_H */

