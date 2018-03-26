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

#ifndef SIGNALSET_H
#define SIGNALSET_H

#include "DataRow.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

class SignalData;
class DurationSpecification;

enum TimeCounter {
  VITAL, WAVE, EITHER
};

class SignalSet {
public:
  SignalSet( );
  virtual ~SignalSet( );
  const std::map<std::string, std::unique_ptr<SignalData>>&vitals( ) const;
  const std::map<std::string, std::unique_ptr<SignalData>>&waves( ) const;
  std::vector<std::reference_wrapper<const std::unique_ptr<SignalData>>>allsignals( ) const;
  std::map<std::string, std::unique_ptr<SignalData>>&vitals( );
  std::map<std::string, std::unique_ptr<SignalData>>&waves( );
  std::map<std::string, std::string>& metadata( );
  const std::map<std::string, std::string>& metadata( ) const;

  /**
   * Adds a new vital sign if it has not already been added. If it already
   * exists, the old dataset is returned
   * @param name
   * @param if not NULL, will be set to true if this is the first time this function
   *  has been called for this vital
   * @return
   */
  std::unique_ptr<SignalData>& addVital( const std::string& name, bool * added = NULL );
  /**
   * Adds a new waveform if it has not already been added. If it already
   * exists, the old dataset is returned
   * @param name
   * @param if not NULL, will be set to true if this is the first time this function
   *  has been called for this vital
   * @return
   */
  std::unique_ptr<SignalData>& addWave( const std::string& name, bool * added = NULL );
  void addMeta( const std::string& key, const std::string& val );
  void reset( bool signalDataOnly = true );
  void setFileSupport( bool );
  dr_time earliest( const TimeCounter& tc = EITHER ) const;
  dr_time latest( const TimeCounter& tc = EITHER ) const;
  void setMetadataFrom( const SignalSet& target );
  const std::map<long, dr_time>& offsets( ) const;
  void addOffset( long seg, dr_time time );
  void clearOffsets( );
  void moveTo( SignalSet& dest );
  /**
   * Sets the valid duration for this signal set. This value will alter the
   * way SignalData instances are created, so it should be set before
   * creating any of them.
   * @param
   */
  void validDuration( const DurationSpecification& );

private:
  SignalSet( const SignalSet& );
  SignalSet operator=(const SignalSet&);

  std::map<std::string, std::unique_ptr<SignalData>> vmap;
  std::map<std::string, std::unique_ptr<SignalData>> wmap;
  std::map<std::string, std::string> metamap;
  std::map<long, dr_time> segs; // segment index->time
  bool largefile;
  std::unique_ptr<DurationSpecification> duration;
};


#endif /* SIGNALSET_H */

