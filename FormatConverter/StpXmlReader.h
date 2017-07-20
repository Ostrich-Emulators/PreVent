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
#include <zlib.h>
#include <libxml/parser.h>

#include "DataRow.h"
#include "StreamChunkReader.h"
#include <libxml/xmlreader.h>

class SignalData;

enum StpXmlReaderState {
	OTHER, HEADER, SEGMENT, VITAL, WAVE
};

class StpXmlReader : public Reader {
public:
	StpXmlReader( );
	virtual ~StpXmlReader( );

protected:
	ReadResult fill( ReadInfo&, const ReadResult& lastfill ) override;
	int getSize( const std::string& input ) const override;
	int prepare( const std::string& input, ReadInfo& info ) override;
	void finish( ) override;

private:
	StpXmlReader( const StpXmlReader& orig );

	ReadResult processNode( ReadInfo& info );

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
	void handleVitalsSet( ReadInfo& info );

	void handleWaveformSet( ReadInfo& info );

	DataRow handleOneVs( std::string& param, std::string& uom );

	ReadResult handleSegmentPatientName( ReadInfo& info );

	/**
	 * Checks if we should rollover given a VitalSigns or Waveforms element. As
	 * a side-effect, sets currtime, and possibly firsttime
	 * @param reader
	 * @return true, if we should rollover
	 */
	bool isRollover( );

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
	std::string trim( std::string& totrim ) const;

	/**
	 * Reads the next element, discarding any junk whitespace before it
	 * @param reader
	 * @return 
	 */
	std::string nextelement( );
	std::string stringAndFree( xmlChar * chars ) const;
	int next( );

	static const std::string MISSING_VALUESTR;

	xmlTextReaderPtr reader;
	time_t prevtime;
	time_t currtime;
	StpXmlReaderState state;
	std::map<std::string, std::string> savedmeta;
};

#endif /* STPXMLREADER_H */

