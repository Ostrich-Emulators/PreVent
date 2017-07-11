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

#ifndef ZLREADER_H
#define ZLREADER_H

#include "FromReader.h"
#include <map>
#include <string>
#include <memory>
#include <istream>
#include <zlib.h>

class SignalData;
class ZlStream;

enum zlReaderState {
	IN_HEADER, IN_VITAL, IN_WAVE, IN_TIME
};

class ZlReader : public FromReader {
public:
	static const int CHUNKSIZE;

	ZlReader( );
	virtual ~ZlReader( );

	int convert( const std::string& input );
	int convert( std::istream&, bool compressed = false );

protected:
	ReadResult readChunk( ReadInfo& );
	int getSize( const std::string& input ) const;

	int prepare( const std::string& input, ReadInfo& info );
	void finish( );

private:

	ZlReader( const ZlReader& orig );

	bool firstread;
	std::string leftoverText;
	time_t currentTime;
	zlReaderState state;
	std::unique_ptr<ZlStream> stream;

	void handleInputChunk( std::string& chunk, ReadInfo& info );
	void handleOneLine( const std::string& chunk, ReadInfo& info );

	static const std::string HEADER;
	static const std::string VITAL;
	static const std::string WAVE;
	static const std::string TIME;
};

class ZlStream {
public:
	ZlStream( std::istream * input, bool compressed, bool isStdin );

	virtual ~ZlStream( );
	void close( );

	std::string readNextChunk( );
	ReadResult getCode( );

	ReadResult rr;
private:
	std::string readNextCompressedChunk( );
	void initZlib( );

	bool iscompressed;
	bool usestdin;

	std::istream * stream;

	// zlib-only var
	z_stream strm;
	unsigned char in[16384 * 16];

};

#endif /* ZLREADER_H */

