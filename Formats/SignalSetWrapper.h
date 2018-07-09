/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignalSetWrapper.h
 * Author: ryan
 *
 * Created on July 2, 2018, 3:08 PM
 */

#ifndef SIGNALSETWRAPPER_H
#define SIGNALSETWRAPPER_H

#include "SignalSet.h"

#include <memory>

class SignalSetWrapper : public SignalSet {
public:
  SignalSetWrapper( const std::unique_ptr<SignalSet>& ss );
  SignalSetWrapper( SignalSet * ss );
  virtual ~SignalSetWrapper( );

  /**
   * Adds a new vital sign if it has not already been added. If it already
   * exists, the old dataset is returned
   * @param name
   * @param if not NULL, will be set to true if this is the first time this function
   *  has been called for this vital
   * @return
   */
  virtual std::unique_ptr<SignalData>& addVital( const std::string& name, bool * added = NULL ) override;
  /**
   * Adds a new waveform if it has not already been added. If it already
   * exists, the old dataset is returned
   * @param name
   * @param if not NULL, will be set to true if this is the first time this function
   *  has been called for this vital
   * @return
   */
  virtual std::unique_ptr<SignalData>& addWave( const std::string& name, bool * added = NULL ) override;

//  virtual std::vector<std::reference_wrapper<const std::unique_ptr<SignalData>>>vitals( ) const override;
//  virtual std::vector<std::reference_wrapper<const std::unique_ptr<SignalData>>>waves( ) const override;
//  virtual std::vector<std::reference_wrapper<std::unique_ptr<SignalData>>>vitals( ) override;
//  virtual std::vector<std::reference_wrapper<std::unique_ptr<SignalData>>>waves( ) override;

  virtual SignalDataIterator begin( ) override;
  virtual SignalDataIterator end( ) override;
  virtual SignalDataIterator vbegin( ) override;
  virtual SignalDataIterator vend( ) override;
  virtual SignalDataIterator wbegin( ) override;
  virtual SignalDataIterator wend( ) override;


  virtual  void reset( bool signalDataOnly = true ) override;
  virtual dr_time earliest( const TimeCounter& tc = EITHER ) const override;
  virtual dr_time latest( const TimeCounter& tc = EITHER ) const override;


private:
  const std::unique_ptr<SignalSet>& set;
};

#endif /* SIGNALSETWRAPPER_H */

