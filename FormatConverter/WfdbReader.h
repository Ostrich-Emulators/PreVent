/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WfdbReader.h
 * Author: ryan
 *
 * Created on July 7, 2017, 2:57 PM
 */

#ifndef WFDBREADER_H
#define WFDBREADER_H

#include "FromReader.h"
#include <ctime>

class WfdbReader : public FromReader {
public:
	static time_t convert( const char * timestr );

protected:
	void doRead( const std::string& input );
	int getSize( const std::string& input ) const;
};

#endif /* WFDBREADER_H */

