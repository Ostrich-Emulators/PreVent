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
  BasicSignalData( const std::string& name, bool iswave = false );
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

  virtual const std::deque<dr_time> times( ) const override;

  virtual double highwater( ) const override;
  virtual double lowwater( ) const override;

  virtual void setMetadataFrom( const SignalData& model ) override;
  virtual void setMeta( const std::string& key, const std::string& val ) override;
  virtual void setMeta( const std::string& key, int val ) override;
  virtual void setMeta( const std::string& key, double val ) override;

  virtual void erases( const std::string& key = "" ) override;
  virtual void erasei( const std::string& key = "" ) override;
  virtual void erased( const std::string& key = "" ) override;

  virtual const std::map<std::string, std::string>& metas( ) const override;
  virtual const std::map<std::string, int>& metai( ) const override;
  virtual const std::map<std::string, double>& metad( ) const override;
  virtual std::vector<std::string> extras( ) const override;
  virtual void extras( const std::string& ext ) override;

  virtual void recordEvent( const std::string& eventtype, const dr_time& time ) override;
  virtual std::vector<std::string> eventtypes( ) override;
  virtual std::vector<dr_time> events( const std::string& eventtype ) override;

private:
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
  size_t livecount; // how big is out data list in memory?
  std::list<std::unique_ptr<DataRow>> data;
  std::deque<dr_time> dates;
  std::FILE * file;
  bool popping;
  bool iswave;
  double highval;
  double lowval;
	bool nocache;
  static const int CACHE_LIMIT;

  std::map<std::string, std::string> metadatas;
  std::map<std::string, int> metadatai;
  std::map<std::string, double> metadatad;
  std::set<std::string> extrafields;
  std::map<std::string, std::vector<dr_time>> namedevents;
};

#endif /* DATASETDATACACHE_H */

