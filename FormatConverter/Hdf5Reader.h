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

#ifndef HDF5READER_H
#define HDF5READER_H

#include "Reader.h"
#include <ctime>

class Hdf5Reader : public Reader {
public:
	Hdf5Reader( );
	virtual ~Hdf5Reader( );

protected:
	int prepare( const std::string& input, ReadInfo& info ) override;
	void finish( ) override;

	ReadResult fill( ReadInfo& data, const ReadResult& lastfill ) override;
	int getSize( const std::string& input ) const override;

private:
	Hdf5Reader( const Hdf5Reader& );

};

#endif /* HDF5READER_H */

