/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WfdbWriter.h
 * Author: ryan
 *
 * Created on July 11, 2017, 7:16 AM
 */

#ifndef ZLWRITER_H
#define ZLWRITER_H

#include "Writer.h"


class SignalData;

class ZlWriter : public Writer {
public:
	ZlWriter( );
	virtual ~ZlWriter( );

protected:
	int initDataSet( const std::string& outdir, const std::string& namestart,
			int compression );
	std::string closeDataSet( );
	int drain( ReadInfo& );

private:
	ZlWriter( const ZlWriter& orig );

	std::string filestart;
	std::string filename;
};


#endif /* ZLWRITER_H */

