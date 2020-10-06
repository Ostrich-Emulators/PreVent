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
#include <deque>
#include <tdmspp.h>

#include "BasicSignalSet.h"

namespace TDMS {
  class channel;
}

namespace FormatConverter {
  class SignalData;

  class SignalSaver {
  public:
    bool waiting;
    bool iswave;
    std::string name;
    dr_time lasttime;
    std::deque<double> leftovers;

    virtual ~SignalSaver( );
    SignalSaver( const std::string& name = "", bool wave = false );
    SignalSaver( const SignalSaver& );
    SignalSaver& operator=(const SignalSaver&);
  };

  class TdmsReader : public Reader, TDMS::listener {
  public:
    TdmsReader( );
    virtual ~TdmsReader( );
    //
    //	virtual void newGroup( TdmsGroup * grp ) override;
    //	virtual void newChannel( TdmsChannel * channel ) override;
    //  virtual void newChannelProperties( TdmsChannel * channel ) override;
    //	/**
    //	 * notify listeners of new value.
    //	 * @param channel
    //	 * @param val
    //	 * @return true, if the reader should push this value to its internal data vector
    //	 */
    //	virtual void newValueChunk( TdmsChannel * channel, std::vector<double>& val ) override;

    virtual void data( const std::string&, const unsigned char*, TDMS::data_type_t, size_t ) override;

  protected:
    int prepare( const std::string& input, SignalSet * info ) override;
    ReadResult fill( SignalSet * data, const ReadResult& lastfill ) override;

  private:
    static const unsigned long MAX_WAITING_GAP_MS;
    std::unique_ptr<TDMS::tdmsfile> tdmsfile;
    SignalSet * filler;
    std::map<std::string, SignalSaver> signalsavers;
    size_t last_segment_read;

    bool writeSignalRow( std::vector<double>& doubles, SignalData * signal, dr_time time );

    void initSignal( TDMS::channel *, bool first );

    /**
     * If a signal starts after a rollover will have occurred, then it'll never
     * get to the "waiting" stage before the rollover. This makes the rollover
     * calculations fail
     */
    void handleLateStarters( );
  };
}
#endif /* WFDBREADER_H */

