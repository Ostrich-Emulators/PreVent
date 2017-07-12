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

class SignalData;

class StpXmlReader : public Reader {
public:
	static const int CHUNKSIZE;

	StpXmlReader( );
	virtual ~StpXmlReader( );

	static void start( void * user_data );
	static void finish( void * user_data );
	static void chars( void * user_data, const xmlChar * ch, int len );
	static void startElement( void * user_data, const xmlChar * name, const xmlChar ** attrs );
	static void endElement( void * user_data, const xmlChar * name );

	void append( const std::string& );
	void reset();
	void setElement( const std::string& element, std::map<std::string, std::string>& map );
	const std::string& getElement();

	//ReadInfo& data;
	DataRow current;
protected:
	ReadResult readChunk( ReadInfo& );
	int getSize( const std::string& input ) const;

	int prepare( const std::string& input, ReadInfo& info );
	void finish( );

private:

	StpXmlReader( const StpXmlReader& orig );

	std::string leftoverText;
	std::string element;
	std::map<std::string, std::string> attrs;
};

#endif /* STPXMLREADER_H */

