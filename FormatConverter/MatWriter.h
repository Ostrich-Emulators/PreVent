/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MatWriter.h
 * Author: ryan
 *
 * Created on July 11, 2017, 7:16 AM
 */

#ifndef MATWRITER_H
#define MATWRITER_H

#include "Writer.h"
#include <iostream>
#include <fstream>
#include <ctime>

class SignalData;
class ReadInfo;

class MatWriter : public Writer {
public:
	MatWriter( );
	virtual ~MatWriter( );

protected:
	int initDataSet( const std::string& outdir, const std::string& namestart,
			int compression );
	std::string closeDataSet( );
	int drain( ReadInfo& );

private:
	MatWriter( const MatWriter& orig );

	int writeVitals( std::map<std::string, std::unique_ptr<SignalData>>&data );
	int writeWaves( std::map<std::string, std::unique_ptr<SignalData>>&data );
	void writeheader( );

	std::string fileloc;
	std::ofstream out;
	time_t firsttime;
	ReadInfo * dataptr;
};


#endif /* MATWRITER_H */

