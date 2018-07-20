/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ReadInfo.h
 * Author: ryan
 *
 * Created on July 10, 2017, 8:02 AM
 */

#ifndef BASICSIGNALSET_H
#define BASICSIGNALSET_H

#include "SignalSet.h"

class SignalData;

class BasicSignalSet : public SignalSet {
public:
  BasicSignalSet();
  virtual ~BasicSignalSet( );

  virtual std::vector<std::unique_ptr<SignalData>>&vitals( ) override;
  virtual std::vector<std::unique_ptr<SignalData>>&waves( ) override;

  virtual const std::vector<std::unique_ptr<SignalData>>&vitals( ) const override;
  virtual const std::vector<std::unique_ptr<SignalData>>&waves( ) const override;

  /**
   * Adds a new vital sign if it has not already been added. If it already
   * exists, the old dataset is returned
   * @param name
   * @param if not NULL, will be set to true if this is the first time this function
   *  has been called for this vital
   * @return
   */
  virtual std::unique_ptr<SignalData>& addVital( const std::string& name, bool * added = nullptr ) override;
  /**
   * Adds a new waveform if it has not already been added. If it already
   * exists, the old dataset is returned
   * @param name
   * @param if not NULL, will be set to true if this is the first time this function
   *  has been called for this vital
   * @return
   */
  virtual std::unique_ptr<SignalData>& addWave( const std::string& name, bool * added = nullptr ) override;
  void reset( bool signalDataOnly = true ) override;
  dr_time earliest( const TimeCounter& tc = EITHER ) const override;
  dr_time latest( const TimeCounter& tc = EITHER ) const override;

protected:
  /**
   * A function to actually make the (custom?) signal data object for
   * addWave() and addVital()
   * @param name
   * @param iswave
   * @return
   */
   virtual std::unique_ptr<SignalData> createSignalData( const std::string& name, bool iswave );

private:
  BasicSignalSet( const BasicSignalSet& );
  BasicSignalSet operator=(const BasicSignalSet&);

  std::vector<std::unique_ptr<SignalData>> vits;
  std::vector<std::unique_ptr<SignalData>> wavs;
};


#endif /* BASICSIGNALSET_H */

