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
#include <TdmsParser.h>
#include <TdmsListener.h>

#include "BasicSignalSet.h"

class SignalData;

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
	virtual bool newValue( TdmsChannel * channel, double val ) override;


protected:
	int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
	void finish( ) override;

	ReadResult fill( std::unique_ptr<SignalSet>& data, const ReadResult& lastfill ) override;

private:
	std::unique_ptr<TdmsParser> parser;
	BasicSignalSet saved;
	SignalSet * filler;
	dr_time lastSaveTime;

	void startSaving( dr_time now );
  void copySavedInto( std::unique_ptr<SignalSet>& newset );
	std::map<TdmsChannel *, dr_time> lastTimes;

	static dr_time parsetime( const std::string& timestr );
	bool writeWaveChunkAndReset( int& count, int& nancount, std::vector<double>& doubles,
			bool& seenFloat, std::unique_ptr<SignalData>& signal, dr_time& time, int timeinc );
};

#endif /* WFDBREADER_H */

