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
	ReadResult readChunk( ReadInfo& );
	int getSize( const std::string& input ) const;
	int prepare( const std::string& input, ReadInfo& info );
	void finish( );

private:
	StpXmlReader( const StpXmlReader& orig );

	ReadResult processNode( xmlTextReaderPtr reader, ReadInfo& info );

	/**
	 * Gets the next text element from the reader, when you've already opened the
	 * element, and you know it has no other children
	 * @param
	 * @return
	 */
	std::string text( xmlTextReaderPtr reader ) const;

	/**
	 * Gets a DataRow from the VS element
	 * @param reader
	 * @return
	 */
	DataRow getVital( xmlTextReaderPtr reader ) const;
	std::map<std::string, std::string> getAttrs( xmlTextReaderPtr reader ) const;
	std::map<std::string, std::string> getHeaders( xmlNodePtr node ) const;
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
	std::string nextelement( xmlTextReaderPtr reader ) const;
	std::string stringAndFree( xmlChar * chars ) const;

	static const std::string MISSING_VALUESTR;

	xmlTextReaderPtr reader;
	DataRow current;
	time_t firsttime;
	time_t prevtime;
	StpXmlReaderState state;

};

#endif /* STPXMLREADER_H */

