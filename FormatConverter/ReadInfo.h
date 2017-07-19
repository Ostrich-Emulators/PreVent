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

#ifndef READINFO_H
#define READINFO_H

#include "DataRow.h"
#include <memory>
#include <string>
#include <map>

class SignalData;

class ReadInfo {
public:
	ReadInfo( );
	virtual ~ReadInfo( );
	std::map<std::string, std::unique_ptr<SignalData>>&vitals( );
	std::map<std::string, std::unique_ptr<SignalData>>&waves( );
	std::map<std::string, std::string>& metadata( );

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

private:
	ReadInfo( const ReadInfo& );
	ReadInfo operator=(const ReadInfo&);

	std::map<std::string, std::unique_ptr<SignalData>> vmap;
	std::map<std::string, std::unique_ptr<SignalData>> wmap;
	std::map<std::string, std::string> metamap;
	bool largefile;
};


#endif /* READINFO_H */

