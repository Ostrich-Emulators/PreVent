/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataSetDataCache.h
 * Author: ryan
 *
 * Created on August 3, 2016, 7:47 AM
 */

#ifndef SIGNALDATA_H
#define SIGNALDATA_H

#include <list>
#include <memory>
#include <string>
#include <time.h>
#include <map>
#include <deque>
#include <set>
#include <vector>
#include "dr_time.h"

class DataRow;

class SignalData {
public:
  static const std::string SCALE;
  static const std::string UOM;
  static const std::string MSM;
  static const std::string TIMEZONE;
  static const std::string LABEL;
  static const std::string BUILD_NUM;
  static const std::string STARTTIME;

  static const std::string CHUNK_INTERVAL_MS;
  static const std::string READINGS_PER_CHUNK;

  static const std::string MISSING_VALUESTR;

  static const short MISSING_VALUE;

  SignalData( );
  virtual ~SignalData( );

  virtual std::unique_ptr<SignalData> shallowcopy( bool includedates = false ) = 0;
  virtual void add( const DataRow& row ) = 0;
  virtual size_t size( ) const = 0;
  virtual dr_time startTime( ) const = 0;
  virtual dr_time endTime( ) const = 0;
  virtual const std::string& name( ) const = 0;
  /**
   * Gets the dr_times for this dataset.
   * @param offset_ms apply this offset (in ms) to every time (like for different timezones)
   *
   * @return
   */
  virtual const std::deque<dr_time> times( ) const = 0;

  virtual std::unique_ptr<DataRow> pop( ) = 0;
  virtual void setWave( bool wave = false ) = 0;
  virtual bool wave( ) const = 0;
  /**
   * Retrieves the highest value in this signal data
   * @return
   */
  virtual double highwater( ) const = 0;
  /**
   * Retrieves the lowest value in this signal data
   * @return
   */
  virtual double lowwater( ) const = 0;

  virtual void setUom( const std::string& u );
  virtual const std::string& uom( ) const;
  virtual double hz( ) const;
  virtual int readingsPerSample( ) const;
  virtual void setChunkIntervalAndSampleRate( int chunktime_ms, int samplerate );
  virtual bool empty( ) const;
  virtual void moveDataTo( std::unique_ptr<SignalData>& signal );
  virtual void scale( int scaling );
  virtual int scale( ) const;

  virtual void setMetadataFrom( const SignalData& model ) = 0;
  virtual void setMeta( const std::string& key, const std::string& val ) = 0;
  virtual void setMeta( const std::string& key, int val ) = 0;
  virtual void setMeta( const std::string& key, double val ) = 0;

  virtual void erases( const std::string& key = "" ) = 0;
  virtual void erasei( const std::string& key = "" ) = 0;
  virtual void erased( const std::string& key = "" ) = 0;

  virtual const std::map<std::string, std::string>& metas( ) const = 0;
  virtual const std::map<std::string, int>& metai( ) const = 0;
  virtual const std::map<std::string, double>& metad( ) const = 0;
  virtual std::vector<std::string> extras( ) const = 0;

  virtual void extras( const std::string& ext ) = 0;

  /**
   * Creates an "event" data point at the given time. This is intended to
   * give us a place to store non-invasive blood pressures, which are performed
   * manually at arbitrary intervals. Consecutive events with the same time
   * will be ignored
   * @param eventtype what event we're marking (this will be the dataset name)
   * @param time when the event happened
   *
   */
  virtual void recordEvent( const std::string& eventtype, const dr_time& time ) = 0;
  virtual std::vector<std::string> eventtypes( ) = 0;
  virtual std::vector<dr_time> events( const std::string& eventtype ) = 0;

  /**
   * How many DataRows are in memory (not cached to disk)
   * @return
   */
  virtual size_t inmemsize( ) const = 0;
};

#endif /* SIGNALDATA_H */

