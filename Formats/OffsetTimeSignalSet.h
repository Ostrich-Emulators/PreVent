/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LocaltimeSignalSet.h
 * Author: ryan
 *
 * Created on July 2, 2018, 3:16 PM
 */

#ifndef OFFSETSIGNALSET_H
#define OFFSETSIGNALSET_H

#include "SignalSetWrapper.h"
#include "SignalDataWrapper.h"

class OffsetTimeSignalSet : public SignalSetWrapper {
public:
  static const std::string COLLECTION_TIMEZONE;
  static const std::string COLLECTION_TIMEZONE_OFFSET;

  OffsetTimeSignalSet( const std::string& name, long offset_ms = 0 );
  OffsetTimeSignalSet( const std::unique_ptr<SignalSet>& w, const std::string& name,
      long offset_ms = 0 );
  OffsetTimeSignalSet( SignalSet * w, const std::string& name, long offset_ms = 0 );
  ~OffsetTimeSignalSet( );

  virtual std::unique_ptr<SignalData>& addVital( const std::string& name, bool * added = nullptr ) override;
  virtual std::unique_ptr<SignalData>& addWave( const std::string& name, bool * added = nullptr ) override;
  virtual void reset( bool signalDataOnly = false ) override;

private:
  std::string tz_name;
  long tz_offset_ms;
};

#endif /* OFFSETSIGNALSET_H */

