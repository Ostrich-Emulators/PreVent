/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClippingSignalSet.h
 * Author: ryan
 *
 * Created on September 26, 2018, 9:49 AM
 */

#ifndef CLIPPINGSIGNALSET_H
#define CLIPPINGSIGNALSET_H

#include "SignalSetWrapper.h"

class ClippingSignalSet : public SignalSetWrapper {
public:
  ClippingSignalSet( dr_time * starttime = nullptr, dr_time * endtime = nullptr );
  ClippingSignalSet( const std::unique_ptr<SignalSet>& w, dr_time * starttime = nullptr, dr_time * endtime = nullptr );
  ClippingSignalSet( SignalSet * w, dr_time * starttime = nullptr, dr_time * endtime = nullptr );
  ~ClippingSignalSet( );

  /**
   * Gets a ClippingSignalSet that only allows data points for the given duration (starting with the first data point)
   * @param for_ms the number of ms to allow
   * @return the set
   */
  static std::unique_ptr<ClippingSignalSet> duration( const dr_time& for_ms, dr_time * starttime = nullptr );

  virtual std::unique_ptr<SignalData>& addVital( const std::string& name, bool * added = nullptr ) override;
  virtual std::unique_ptr<SignalData>& addWave( const std::string& name, bool * added = nullptr ) override;

  bool timeok( const dr_time& time );

private:
  void init( dr_time * starttime, dr_time * endtime );
  void initForDuration( const dr_time& duration_ms );

  dr_time start;
  dr_time end;
  bool checkstart;
  bool checkend;

  dr_time duration_ms;
  bool havefirsttime;
  bool checkduration;
};

#endif /* CLIPPINGSIGNALSET_H */

