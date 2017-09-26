/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CacheFileHdf5Writer.h
 * Author: ryan
 *
 * Created on August 26, 2016, 12:55 PM
 */

#ifndef STPXMLREADER_H
#define STPXMLREADER_H

#include "Reader.h"
#include <string>
#include <list>
#include <set>
#include <zlib.h>

#include "DataRow.h"
#include "SignalSet.h"
#include "StreamChunkReader.h"
#include <expat.h>
#include <iostream>
#include <fstream>

class SignalData;

class StpXmlReader : public Reader {
public:
	static const std::set<std::string> Hz60;
	static const std::set<std::string> Hz120;
	static const std::string MISSING_VALUESTR;
	static const int INHEADER;
	static const int INVITAL;
	static const int INWAVE;
	static const int INNAME;
	static const int INDETERMINATE;

	StpXmlReader( );
	virtual ~StpXmlReader( );

protected:
	ReadResult fill( SignalSet&, const ReadResult& lastfill ) override;
	size_t getSize( const std::string& input ) const override;
	int prepare( const std::string& input, SignalSet& info ) override;
	void finish( ) override;

private:
	StpXmlReader( const StpXmlReader& orig );

	static void start( void * data, const char * el, const char ** attr );
	static void end( void * data, const char * el );
	static void chars( void * data, const char * text, int len );
	static std::string resample( const std::string& data, int hz );
	static bool waveIsOk( const std::string& wavedata );

	/**
	 * Trims the given string in-place, and returns it
	 * @param totrim
	 * @return
	 */
	static std::string trim( std::string& totrim );

	void copysaved( SignalSet& newset );
	void start( const std::string& element, std::map<std::string, std::string>& attrs );
	void end( const std::string& element, const std::string& text );

	void setstate( int state );
	time_t time( const std::string& val ) const;
	bool isRollover( const time_t& now, const time_t& then ) const;

	XML_Parser parser;
	time_t prevtime;
	time_t currvstime;
	time_t lastvstime;
	time_t currwavetime;
	time_t lastwavetime;
	bool warnMissingName;
	bool warnJunkData;

	static std::string working;
	SignalSet saved;
	SignalSet * filler;
	std::ifstream input;
	ReadResult rslt;
	int state;
	std::string label;
	std::string value;
	std::string uom;
	std::string q;
};

#endif /* STPXMLREADER_H */

