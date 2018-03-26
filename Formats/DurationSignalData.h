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

#include "SignalData.h"
class DurationSpecification;


class DurationSignalData : public SignalData {
public:
  DurationSignalData( const std::string& name, const DurationSpecification& spec,
      bool largefilesupport = false, bool iswave = false );
  void add( const DataRow& row ) override;
private:
  const DurationSpecification& duration;
};

#endif /* DURATIONSIGNALDATA_H */

