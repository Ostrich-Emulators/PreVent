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
#include <map>
#include <string>
#include <memory>
#include <istream>
#include <zlib.h>

class SignalData;

class StpXmlReader : public Reader {
public:
	static const int CHUNKSIZE;

	StpXmlReader( );
	virtual ~StpXmlReader( );

protected:
	ReadResult readChunk( ReadInfo& );
	int getSize( const std::string& input ) const;

	int prepare( const std::string& input, ReadInfo& info );
	void finish( );

private:

	StpXmlReader( const StpXmlReader& orig );

	std::string leftoverText;
};
#endif /* STPXMLREADER_H */

