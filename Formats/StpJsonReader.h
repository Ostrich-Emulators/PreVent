/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CacheFileHdf5Writer.h
 * Author: ryan
 *
 * Created on August 26, 2016, 12:55 PM
 */

#ifndef STPJSONREADER_H
#define STPJSONREADER_H

#include "Reader.h"
#include <map>
#include <string>
#include <memory>
#include <istream>
#include <zlib.h>

class SignalData;
class StreamChunkReader;

enum jsonReaderState {
	JIN_HEADER, JIN_VITAL, JIN_WAVE, JIN_TIME
};

class StpJsonReader : public Reader {
public:
	static const int CHUNKSIZE;

	StpJsonReader( );
	virtual ~StpJsonReader( );

protected:
	ReadResult fill( std::unique_ptr<SignalSet>&, const ReadResult& lastfill ) override;

	int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
	void finish( ) override;

private:

	StpJsonReader( const StpJsonReader& orig );

	bool firstread;
	std::string leftoverText;
	time_t currentTime;
	jsonReaderState state;
	std::unique_ptr<StreamChunkReader> stream;

	void handleInputChunk( std::string& chunk, std::unique_ptr<SignalSet>& info );
	void handleOneLine( const std::string& chunk, std::unique_ptr<SignalSet>& info );

	static const std::string HEADER;
	static const std::string VITAL;
	static const std::string WAVE;
	static const std::string TIME;
};

#endif /* STPJSONREADER_H */

