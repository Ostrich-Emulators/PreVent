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

#ifndef DURATIONSIGNALDATA_H
#define DURATIONSIGNALDATA_H

#include "SignalDataWrapper.h"

class DurationSpecification;

/**
 * A wrapper class around some other SignalData that specifies a valid
 * duration. DateRows that fall outside the valid specification are silently ignored
 * @param data
 * @param spec
 */
class DurationSignalData : public SignalDataWrapper {
public:
  DurationSignalData( std::unique_ptr<SignalData> data, const DurationSpecification& spec);
  virtual ~DurationSignalData();

  virtual void add(const DataRow& row) override;
  
private:
  const DurationSpecification& spec;
};

#endif /* DURATIONSIGNALDATA_H */

