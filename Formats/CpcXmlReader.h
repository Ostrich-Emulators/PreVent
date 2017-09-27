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

#ifndef CPCXMLREADER_H
#define CPCXMLREADER_H

#include "XmlReaderBase.h"
#include <string>
#include <list>
#include <set>
#include <zlib.h>


#include "DataRow.h"
#include "StreamChunkReader.h"

class SignalData;

class CpcXmlReader : public XmlReaderBase {
public:

	CpcXmlReader( );
	virtual ~CpcXmlReader( );

private:
	CpcXmlReader( const CpcXmlReader& orig );

protected:
	virtual void start( const std::string& element, std::map<std::string, std::string>& attrs );
	virtual void end( const std::string& element, const std::string& text );
};

#endif /* CPCXMLREADER_H */
