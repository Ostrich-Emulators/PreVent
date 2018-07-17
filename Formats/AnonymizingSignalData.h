/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DurationSignalData.h
 * Author: ryan
 *
 * Created on March 26, 2018, 4:39 PM
 */

#ifndef ANONYMIZINGSIGNALDATA_H
#define ANONYMIZINGSIGNALDATA_H

#include "SignalDataWrapper.h"
#include <string>
#include <deque>

/**
 * A wrapper class around some other SignalData that specifies a different
 * timezone (DataRows store data as UTC) for time-oriented functions
 * @param data
 * @param spec
 */
class AnonymizingSignalData : public SignalDataWrapper {
public:
  AnonymizingSignalData( SignalData * data, dr_time& firsttime );
  AnonymizingSignalData( const std::unique_ptr<SignalData>& data, dr_time& firsttime );
  virtual ~AnonymizingSignalData( );

  virtual void add( const DataRow& row ) override;

private:
  dr_time& firsttime;
};

#endif /* ANONYMIZINGSIGNALDATA_H */

