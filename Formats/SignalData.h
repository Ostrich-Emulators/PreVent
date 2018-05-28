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
  static const std::string HERTZ;
  static const std::string SCALE;
  static const std::string UOM;
  static const std::string MSM;
  static const std::string TIMEZONE;
  static const std::string VALS_PER_DR;
  static const std::string LABEL;
  static const std::string MISSING_VALUESTR;
  static const short MISSING_VALUE;

  SignalData( );
  SignalData( const SignalData& orig );
  virtual ~SignalData( );

  virtual std::unique_ptr<SignalData> shallowcopy( bool includedates = false ) = 0;
  virtual void add( const DataRow& row ) = 0;
  virtual size_t size( ) const = 0;
  virtual const dr_time& startTime( ) const = 0;
  virtual const dr_time& endTime( ) const = 0;
  virtual const std::string& name( ) const = 0;
  /**
   * Gets the dr_times for this dataset.
   * @param offset_ms apply this offset (in ms) to every time (like for different timezones)
   *
   * @return
   */
  virtual const std::deque<dr_time> times( long offset_ms = 0 ) const = 0;

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
  virtual int scale( ) const;
  virtual double hz( ) const;

  virtual void moveDataTo( std::unique_ptr<SignalData>& signal );
  virtual void setMetadataFrom( const SignalData& model );
  virtual std::map<std::string, std::string>& metas( );
  virtual std::map<std::string, int>& metai( );
  virtual std::map<std::string, double>& metad( );
  virtual const std::map<std::string, std::string>& metas( ) const;
  virtual const std::map<std::string, int>& metai( ) const;
  virtual const std::map<std::string, double>& metad( ) const;
  virtual std::vector<std::string> extras( ) const;
  virtual void setValuesPerDataRow( int );
  virtual int valuesPerDataRow( ) const;
  virtual void scale( int scaling );
  virtual bool empty( ) const;

private:
  std::map<std::string, std::string> metadatas;
  std::map<std::string, int> metadatai;
  std::map<std::string, double> metadatad;
  std::set<std::string> extrafields;
};

#endif /* SIGNALDATA_H */

