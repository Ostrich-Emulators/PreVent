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
	static const std::string HERTZ;
	static const std::string SCALE;
	static const std::string UOM;
	static const std::string MSM;
	static const std::string TIMEZONE;


	SignalData( const std::string& name, bool largefilesupport = false, bool iswave = false );
	std::unique_ptr<SignalData> shallowcopy( );
	virtual ~SignalData( );

	void add( const DataRow& row );
	void setUom( const std::string& u );
	const std::string& uom( ) const;
	int scale( ) const;
	int size( ) const;
	double hz( ) const;
	const time_t& startTime( ) const;
	const time_t& endTime( ) const;
	const std::string& name( ) const;

	std::unique_ptr<DataRow> pop( );
	bool empty( ) const;
	void setWave( bool wave = false );
	bool wave( ) const;

	std::map<std::string, std::string>& metas( );
	std::map<std::string, int>& metai( );
	std::map<std::string, double>& metad( );

private:
	SignalData( const SignalData& orig );

	void startPopping( );

	/**
	 * copy rows from the cache file to the data list.
	 * @param count the desired elements to uncache
	 * @return the number uncached, or 0 if there is no cache, or it's empty
	 */
	int uncache( int count = CACHE_LIMIT );
	void cache( );
	void setScale( int x );


	const std::string label;
	time_t firstdata;
	time_t lastdata;
	int datacount;
	std::list<std::unique_ptr<DataRow>> data;
	std::map<std::string, std::string> metadatas;
	std::map<std::string, int> metadatai;
	std::map<std::string, double> metadatad;
	std::FILE * file;
	bool popping;
	bool iswave;

	static const int CACHE_LIMIT;
};

#endif /* DATASETDATACACHE_H */

