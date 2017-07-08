/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   H5Writer.h
 * Author: ryan
 *
 * Created on July 8, 2017, 2:41 PM
 */

#ifndef HDFWRITER_H
#define HDFWRITER_H

#include "ToWriter.h"
#include <memory>

class FromReader;

class HdfWriter : public ToWriter {
public:
	HdfWriter( );
	void write( std::unique_ptr<FromReader>& from );

private:

	HdfWriter( const HdfWriter& ) {
	}

};


#endif /* HWRITER_H */

