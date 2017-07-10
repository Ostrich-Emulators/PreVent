/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Formats.h
 * Author: ryan
 *
 * Created on July 7, 2017, 2:24 PM
 */

#ifndef FORMATS_H
#define FORMATS_H

#include <string>

// valid formats
enum Format {
	UNRECOGNIZED, WFDB, HDF5, STPXML, DSZL
};

class Formats {
public:
	static Format getValue( const std::string& fmt );
	virtual ~Formats( );
private:
	Formats( );
	Formats( const Formats& );
};

#endif /* FORMATS_H */

