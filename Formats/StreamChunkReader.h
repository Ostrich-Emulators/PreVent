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
#include <vector>

#include <zlib.h>
#include <zip.h>

#include "Reader.h"

/**
 * Class to read either raw or zlib-compressed text from either stdin or a file.
 */
namespace FormatConverter {

  enum StreamType {
    RAW, COMPRESS, GZIP, ZIP
  };

  class StreamChunkReader {
  public:

    static std::unique_ptr<StreamChunkReader> fromStdin( StreamType t = StreamType::RAW,
        int chunksize = DEFAULT_CHUNKSIZE );
    static std::unique_ptr<StreamChunkReader> fromFile( const std::string& filename,
        int chunksize = DEFAULT_CHUNKSIZE );

    virtual ~StreamChunkReader( );
    void close( );

    /**
     * Reads this many bytes 
     * @param numbytes
     * @return
     */
    std::string read( int numbytes );
    std::string readNextChunk( );
    /**
     * Reads this many bytes into the given vector. Note that if this is a
     * compressed stream, this function reads in this many compressed bytes,
     * so the output size may be much larger
     * @param vec
     * @return the number of bytes read
     */
    int read( std::vector<char>& vec, int numbytes );
    void setChunkSize( int size );

    ReadResult rr;
  private:

    StreamChunkReader( std::istream * input, bool closestream,
        StreamType ziptype = StreamType::COMPRESS,
        int chunksize = DEFAULT_CHUNKSIZE );
    StreamChunkReader( const std::string& filename,
        StreamType ziptype = StreamType::COMPRESS,
        int chunksize = DEFAULT_CHUNKSIZE );

    static const int DEFAULT_CHUNKSIZE;

    std::string readNextCompressedChunk( int numbytes );
    void initZlib( bool forGzip = false );

    bool closeStreamAtEnd;
    int chunksize;

    StreamType type;

    // zlib-only vars
    z_stream strm;
    std::istream * stream;

    // libzip-only vars
    zip_t * ziparchive;
    zip_file_t * zipdata;
  };
}
#endif /* FILEORCINSTREAM_H */

