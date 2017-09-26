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
#include <libxml/parser.h>

#include "DataRow.h"
#include "SignalSet.h"
#include "StreamChunkReader.h"
#include <expat.h>
#include <iostream>
#include <fstream>
#include <libxml/xmlreader.h>

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

	ReadResult processNode( SignalSet& info );

	static std::string resample( const std::string& data, int hz );

	/**
	 * Gets the next text element from the reader, when you've already opened the
	 * element, and you know it has no other children
	 * @param
	 * @return
	 */
	std::string text( );

	/**
	 * Handles a set of vitals. The Reader is expected to be at a VitalSigns element
	 * @param info where to save the data
	 */
	void handleVitalsSet( SignalSet& info );

	void handleWaveformSet( SignalSet& info );

	DataRow handleOneVs( std::string& param, std::string& uom );

	ReadResult handleSegmentPatientName( SignalSet& info );

	/**
	 * Checks if we should rollover given a VitalSigns or Waveforms element. As
	 * a side-effect, sets currtime, and possibly firsttime
	 * @param reader
	 * @return true, if we should rollover
	 */
	bool isRollover( bool forVitals );

	std::map<std::string, std::string> getAttrs( );
	std::map<std::string, std::string> getHeaders( );

	/**
	 * Gets the next text content of the node and moves the reader to the close tag
	 * @return
	 */
	std::string textAndClose( );

	/**
	 * Gets the text and any attributes from this node. Assumes the reader is
	 * on the opening element
	 * @param attrs
	 * @return
	 */
	std::string textAndAttrsToClose( std::map<std::string, std::string>& attrs );

	/**
	 * Trims the given string in-place. Also returns that same string
	 * @param totrim
	 * @return the argument
	 */
	static std::string trim( std::string& totrim );

	/**
	 * Reads the next element, discarding any junk whitespace before it
	 * @param reader
	 * @return 
	 */
	std::string nextelement( );
	std::string stringAndFree( xmlChar * chars ) const;
	int next( );

	static bool waveIsOk( const std::string& wavedata );

	void copysaved( SignalSet& newset );
	void start( const std::string& element, std::map<std::string, std::string>& attrs );
	void end( const std::string& element, const std::string& text );

	void setstate( int state );
	time_t time( const std::string& val ) const;
	bool isRollover( const time_t& now, const time_t& then ) const;

	xmlTextReaderPtr reader;
	XML_Parser parser;
	time_t prevtime;
	time_t currvstime;
	time_t lastvstime;
	time_t currwavetime;
	time_t lastwavetime;
	bool warnMissingName;
	bool warnJunkData;

	static std::string working;
	// std::map<std::string, std::string> savedmeta;
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

