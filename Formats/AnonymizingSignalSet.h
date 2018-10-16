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
#include "FileNamer.h"

#include <map>

class AnonymizingSignalSet : public SignalSetWrapper {
public:
  static const std::string DEFAULT_FILENAME_PATTERN;
  AnonymizingSignalSet( FileNamer& filenamer );
  AnonymizingSignalSet( const std::unique_ptr<SignalSet>& w, FileNamer& filenamer );
  AnonymizingSignalSet( SignalSet * w, FileNamer& filenamer );
  ~AnonymizingSignalSet( );

  virtual std::unique_ptr<SignalData>& addVital( const std::string& name, bool * added = nullptr ) override;
  virtual std::unique_ptr<SignalData>& addWave( const std::string& name, bool * added = nullptr ) override;

  virtual void setMeta( const std::string& key, const std::string& val ) override;

  virtual void complete( ) override;

private:
  FileNamer& namer;
  dr_time firsttime;
  std::map<std::string, std::string> saveddata;
};

#endif /* ANONYMIZINGSIGNALSET_H */

