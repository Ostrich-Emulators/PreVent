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

#include "dr_time.h"

class WfdbReader : public Reader {
public:
	WfdbReader( );

	virtual ~WfdbReader( );

	static dr_time convert( const char * timestr );

protected:
	int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
	void finish( ) override;

	ReadResult fill( std::unique_ptr<SignalSet>& data, const ReadResult& lastfill ) override;

private:
	int sigcount;
	WFDB_Siginfo * siginfo;

};

#endif /* WFDBREADER_H */

