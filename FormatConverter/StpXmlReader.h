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
#include <zlib.h>
#include <libxml/parser.h>

#include "DataRow.h"
#include "StreamChunkReader.h"

class SignalData;

enum StpXmlReaderState { OTHER, HEADER, VITAL, WAVE };

class StpXmlReader : public Reader {
public:
	StpXmlReader( );
	virtual ~StpXmlReader( );

	static void start( void * user_data );
	static void finish( void * user_data );
	static void chars( void * user_data, const xmlChar * ch, int len );
	static void startElement( void * user_data, const xmlChar * name, const xmlChar ** attrs );
	static void endElement( void * user_data, const xmlChar * name );
	static void error( void *user_data, const char *msg, ... );

protected:
	ReadResult readChunk( ReadInfo& );
	int getSize( const std::string& input ) const;
	int prepare( const std::string& input, ReadInfo& info );
	void finish( );

private:
	StpXmlReader( const StpXmlReader& orig );

	std::unique_ptr<StreamChunkReader> stream;
	xmlParserCtxtPtr context;
	static ReadInfo& convertUserDataToReadInfo( void * data );

	static ReadResult handleVital( const std::string& element, ReadInfo& );
	static ReadResult handleWave( const std::string& element, ReadInfo& );

	static const std::string MISSING_VALUESTR;

	static std::string workingText;
	static std::string element;
	static std::string last; // some sort of data we'll need later in the parsing
	static std::map<std::string, std::string> attrs;
	static ReadResult rslt;
	static DataRow current;
	static StpXmlReaderState state;
	static time_t firsttime;
};

#endif /* STPXMLREADER_H */

