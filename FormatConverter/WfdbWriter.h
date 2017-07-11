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

#ifndef WFDBWRITER_H
#define WFDBWRITER_H

#include "Writer.h"
class SignalData;

class WfdbWriter : public Writer {
public:
	WfdbWriter( );
	virtual ~WfdbWriter( );

protected:
	int initDataSet( const std::string& newfile, int compression );
	std::string closeDataSet( );
	int drain( ReadInfo& );

private:

	WfdbWriter( const WfdbWriter& orig );

};


#endif /* WFDBWRITER_H */

