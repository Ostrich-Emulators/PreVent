/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CacheFileHdf5Writer.cpp
 * Author: ryan
 * 
 * Created on August 26, 2016, 12:55 PM
 * 
 * Almost all the zlib code was taken from http://www.zlib.net/zlib_how.html
 */

#include "CpcXmlReader.h"
#include "SignalData.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <map>

CpcXmlReader::CpcXmlReader( ) : XmlReaderBase( ) {
}

CpcXmlReader::CpcXmlReader( const CpcXmlReader& orig ) : XmlReaderBase( orig ) {
}

CpcXmlReader::~CpcXmlReader( ) {
}

void CpcXmlReader::start( const std::string& element,
    std::map<std::string, std::string>& attrs ) {
}

void CpcXmlReader::end( const std::string& element, const std::string& text ) {
}
