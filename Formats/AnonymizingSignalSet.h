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

#ifndef ANONYMIZINGSIGNALSET_H
#define ANONYMIZINGSIGNALSET_H

#include "SignalSetWrapper.h"
#include "SignalDataWrapper.h"

class AnonymizingSignalSet : public SignalSetWrapper {
public:

  AnonymizingSignalSet( );
  AnonymizingSignalSet( const std::unique_ptr<SignalSet>& w );
  AnonymizingSignalSet( SignalSet * w );
  ~AnonymizingSignalSet( );

  virtual std::unique_ptr<SignalData>& addVital( const std::string& name, bool * added = nullptr ) override;
  virtual std::unique_ptr<SignalData>& addWave( const std::string& name, bool * added = nullptr ) override;

  virtual void setMeta( const std::string& key, const std::string& val ) override;

private:
  dr_time firsttime;
};

class AnonymizingSignalData : public SignalDataWrapper {
public:
  AnonymizingSignalData( SignalData * data, dr_time& firsttime );
  AnonymizingSignalData( const std::unique_ptr<SignalData>& data, dr_time& firsttime );
  virtual ~AnonymizingSignalData( );

  virtual void add( const DataRow& row ) override;

private:
  dr_time& firsttime;
};

#endif /* ANONYMIZINGSIGNALSET_H */

