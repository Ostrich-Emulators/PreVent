/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FileOrCinStream.h
 * Author: ryan
 *
 * Created on July 12, 2017, 3:59 PM
 */

#ifndef FILEORCINSTREAM_H
#define FILEORCINSTREAM_H

#include <string>
#include <iostream>

#include <zlib.h>

#include "Reader.h"

class StreamChunkReader {
public:
	static const int CHUNKSIZE;

	StreamChunkReader( std::istream * input, bool compressed, bool isStdin,
			int chunksize = CHUNKSIZE );

	virtual ~StreamChunkReader( );
	void close( );

	std::string readNextChunk( );
	ReadResult getCode( );

	ReadResult rr;
private:
	std::string readNextCompressedChunk( );
	void initZlib( );

	bool iscompressed;
	bool usestdin;
	int chunksize;

	std::istream * stream;


	// zlib-only var
	z_stream strm;
};

#endif /* FILEORCINSTREAM_H */

