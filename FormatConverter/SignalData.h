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

#include <list>
#include <memory>
#include <string>
#include <time.h>
#include <map>

class DataRow;

class SignalData {
public:
  SignalData( const std::string& name, bool largefilesupport );
  virtual ~SignalData( );

  void add( const DataRow& row );
  void setUom( const std::string& u );
  const std::string& uom( ) const;
  int scale( ) const;
  void setScale( int x );
  int size( ) const;
  const time_t& startTime( ) const;
  const time_t& endTime( ) const;
  const std::string& name( ) const;

  void startPopping();
  std::unique_ptr<DataRow> pop( );

	std::map<std::string, std::string>& metas();

private:
  SignalData( const SignalData& orig );
  /**
   * copy rows from the cache file to the data list.  
   * @param count the desired elements to uncache
   * @return the number uncached, or 0 if there is no cache, or it's empty
   */
  int uncache( int count = CACHE_LIMIT );
  void cache();

  const std::string label;
  std::string _uom;
  time_t firstdata;
  time_t lastdata;
  int datacount;
  int _scale;
  std::list<std::unique_ptr<DataRow>> data;
	std::map<std::string, std::string> metadata;
  std::FILE * file;

  static const int CACHE_LIMIT;
};

#endif /* DATASETDATACACHE_H */

