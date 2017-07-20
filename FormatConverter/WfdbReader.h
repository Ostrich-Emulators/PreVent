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

#include "Reader.h"
#include <ctime>
#include <wfdb/wfdb.h>

class WfdbReader : public Reader {
public:
	static time_t convert( const char * timestr );

protected:
	int prepare( const std::string& input, ReadInfo& info ) override;
	void finish( ) override;

	ReadResult fill( ReadInfo& data, const ReadResult& lastfill ) override;
	int getSize( const std::string& input ) const override;

private:
	int sigcount;
	WFDB_Siginfo * siginfo;

};

#endif /* WFDBREADER_H */

