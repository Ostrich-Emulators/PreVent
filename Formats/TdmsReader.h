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

#ifndef TDMSREADER_H
#define TDMSREADER_H

#include "Reader.h"
#include <memory>
#include <string>
#include <deque>
#include <TdmsParser.h>
#include <TdmsListener.h>

#include "BasicSignalSet.h"

class SignalData;

class SignalSaver {
public:
	bool seenfloat;
	size_t nancount;
	bool waiting;
	bool iswave;
	std::string name;
	dr_time lasttime;
	std::deque<double> leftovers;

	virtual ~SignalSaver( );
	SignalSaver( const std::string& name = "", bool wave = false );
	SignalSaver( const SignalSaver& );
	SignalSaver& operator=(const SignalSaver&);
};

class TdmsReader : public Reader, TdmsListener {
public:
	TdmsReader( );
	virtual ~TdmsReader( );

	virtual void newGroup( TdmsGroup * grp ) override;
	virtual void newChannel( TdmsChannel * channel ) override;
	/**
	 * notify listeners of new value.
	 * @param channel
	 * @param val
	 * @return true, if the reader should push this value to its internal data vector
	 */
	virtual void newValueChunk( TdmsChannel * channel, std::vector<double>& val ) override;


protected:
	int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
	ReadResult fill( std::unique_ptr<SignalSet>& data, const ReadResult& lastfill ) override;

private:
	std::unique_ptr<TdmsParser> parser;
	SignalSet * filler;
	std::map<TdmsChannel *, SignalSaver> signalsavers;

	bool isRollover( const dr_time& now, const dr_time& then ) const;
	static dr_time parsetime( const std::string& timestr );
	bool writeSignalRow( size_t count, size_t nancount, std::vector<double>& doubles,
			const bool seenFloat, const std::unique_ptr<SignalData>& signal, dr_time time );
};

#endif /* WFDBREADER_H */

