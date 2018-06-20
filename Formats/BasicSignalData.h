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

#ifndef DATASETDATACACHE_H
#define DATASETDATACACHE_H

#include "SignalData.h"

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

class BasicSignalData : public SignalData {
public:
  BasicSignalData( const std::string& name, bool largefilesupport = false, bool iswave = false );
  virtual ~BasicSignalData( );

  virtual std::unique_ptr<SignalData> shallowcopy( bool includedates = false ) override;
  virtual void add( const DataRow& row ) override;
  virtual size_t size( ) const override;
  virtual dr_time startTime( ) const override;
  virtual dr_time endTime( ) const override;
  virtual const std::string& name( ) const override;

  virtual std::unique_ptr<DataRow> pop( ) override;
  virtual void setWave( bool wave = false ) override;
  virtual bool wave( ) const override;

  virtual const std::deque<dr_time> times( long offset_ms = 0 ) const override;

  virtual double highwater( ) const override;
  virtual double lowwater( ) const override;

private:
  BasicSignalData( const BasicSignalData& orig );

  void startPopping( );

  /**
   * copy rows from the cache file to the data list.
   * @param count the desired elements to uncache
   * @return the number uncached, or 0 if there is no cache, or it's empty
   */
  int uncache( int count = CACHE_LIMIT );
  void cache( );

  const std::string label;
  dr_time firstdata;
  dr_time lastdata;
  size_t datacount;
  std::list<std::unique_ptr<DataRow>> data;
  std::deque<dr_time> dates;
  std::FILE * file;
  bool popping;
  bool iswave;
  double highval;
  double lowval;
  static const int CACHE_LIMIT;
};

#endif /* DATASETDATACACHE_H */

