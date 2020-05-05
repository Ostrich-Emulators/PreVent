/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CacheFileHdf5Writer.cpp
 * Author: ryan
 * 
 * Created on August 26, 2016, 12:55 PM
 * 
 * Almost all the zlib code was taken from http://www.zlib.net/zlib_how.html
 */

#include "StpPhilipsReader.h"
#include "SignalData.h"
#include "DataRow.h"
#include "Hdf5Writer.h"
#include "StreamChunkReader.h"
#include "BasicSignalSet.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <filesystem>
#include "config.h"
#include "CircularBuffer.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <io.h>
#define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#define SET_BINARY_MODE(file)
#endif

namespace FormatConverter{

  StpPhilipsReader::StpPhilipsReader( const std::string& name ) : StpReaderBase( name ), firstread( true ) { }

  StpPhilipsReader::StpPhilipsReader( const StpPhilipsReader& orig ) : StpReaderBase( orig ), firstread( orig.firstread ) { }

  StpPhilipsReader::~StpPhilipsReader( ) { }

  int StpPhilipsReader::prepare( const std::string& filename, std::unique_ptr<SignalSet>& data ) {
    int rslt = StpReaderBase::prepare( filename, data );
    return rslt;
  }

  ReadResult StpPhilipsReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {
    //output( ) << "initial reading from input stream (popped:" << work.popped( ) << ")" << std::endl;

    int cnt = StpReaderBase::readMore( );

    if ( ReadResult::END_OF_PATIENT == lastrr ) {
      info->setMeta( "Patient Name", "" ); // reset the patient name
    }

    while ( cnt > 0 ) {
      if ( work.available( ) < 1024 * 768 ) {
        // we should never come close to filling up our work buffer
        // so if we have, make sure the sure knows
        std::cerr << "work buffer is too full...something is going wrong" << std::endl;
        return ReadResult::ERROR;
      }

      ChunkReadResult rslt = ChunkReadResult::OK;
      // read as many segments as we can before reading more data
      size_t segsize = 0;
      while ( workHasFullSegment( &segsize ) && ChunkReadResult::OK == rslt ) {
        //output( ) << "next segment is " << std::dec << segsize << " bytes big" << std::endl;

        size_t startpop = work.popped( );
        rslt = processOneChunk( info, segsize );
        size_t endpop = work.popped( );

        if ( ChunkReadResult::OK == rslt ) {
          size_t bytesread = endpop - startpop;
          //output( ) << "read " << bytesread << " bytes of segment" << std::endl;
          if ( bytesread < segsize ) {
            work.skip( segsize - bytesread );
            //output( ) << "skipping ahead " << ( segsize - bytesread ) << " bytes to next segment at " << ( startpop + segsize ) << std::endl;
          }
        }
        else if ( ChunkReadResult::UNKNOWN_BLOCKTYPE == rslt ) {
          return ReadResult::ERROR;
        }
        else {
          // something happened so rewind our to our mark
          //output( ) << "rewinding to start of segment (mark: " << ( work.popped( ) - work.poppedSinceMark( ) ) << ")" << std::endl;
          work.rewindToMark( );

          return ( ChunkReadResult::ROLLOVER == rslt
              ? ReadResult::END_OF_DAY
              : ChunkReadResult::NEW_PATIENT == rslt
              ? ReadResult::END_OF_PATIENT
              : ReadResult::ERROR );
        }
      }

      cnt = readMore( );
      if ( cnt < 0 ) {
        return ReadResult::ERROR;
      }
    }

    //output( ) << "file is exhausted" << std::endl;

    // copy any data we have left in our filler set to the real set


    // if we still have stuff in our work buffer, process it
    if ( !work.empty( ) ) {
      //output( ) << "still have stuff in our work buffer!" << std::endl;
      processOneChunk( info, work.size( ) );
    }
    //copySaved( filler, info );
    return ReadResult::END_OF_FILE;
  }

  StpPhilipsReader::ChunkReadResult StpPhilipsReader::processOneChunk( std::unique_ptr<SignalSet>& info,
      const size_t& maxread ) {
    return ChunkReadResult::OK;
  }

  dr_time StpPhilipsReader::popTime( ) {
    // time is in little-endian format
    auto shorts = work.popvec( 4 );
    time_t time = ( ( shorts[1] << 24 ) | ( shorts[0] << 16 ) | ( shorts[3] << 8 ) | shorts[2] );
    return time * 1000;
  }

  bool StpPhilipsReader::workHasFullSegment( size_t* size ) {
    // ensure the data in our buffer starts with a segment marker, and goes at
    // least to the next segment marker. we do this by reading each byte (sorry)

    if ( work.empty( ) ) {
      return false;
    }

    bool ok = false;
    try {
      work.mark( );
      ok = true;
    }
    catch ( const std::runtime_error& x ) {
      // don't care, but don't throw
    }

    work.rewindToMark( );
    return ok;
  }

  std::vector<StpPhilipsReader::StpMetadata> StpPhilipsReader::parseMetadata( const std::string& input ) {
    std::vector<StpPhilipsReader::StpMetadata> metas;
    std::unique_ptr<SignalSet> info( new BasicSignalSet( ) );
    StpPhilipsReader reader;
    reader.setNonbreaking( true );
    int failed = reader.prepare( input, info );
    if ( failed ) {
      std::cerr << "error while opening input file. error code: " << failed << std::endl;
      return metas;
    }
    reader.setMetadataOnly();

    ReadResult last = ReadResult::FIRST_READ;

    bool okToContinue = true;
    while ( okToContinue ) {
      last = reader.fill( info, last );
      switch ( last ) {
        case ReadResult::FIRST_READ:
          // NOTE: no break here
        case ReadResult::NORMAL:
          break;
        case ReadResult::END_OF_DAY:
          metas.push_back( metaFromSignalSet( info ) );
          info->reset( false );
          break;
        case ReadResult::END_OF_PATIENT:
          metas.push_back( metaFromSignalSet( info ) );
          info->reset( false );
          break;
        case ReadResult::END_OF_FILE:
          metas.push_back( metaFromSignalSet( info ) );
          info->reset( false );
          okToContinue = false;
          break;
        case ReadResult::ERROR:
          std::cerr << "error while reading input file" << std::endl;
          okToContinue = false;
          break;
      }
    }

    reader.finish( );

    return metas;
  }

  StpPhilipsReader::StpMetadata StpPhilipsReader::metaFromSignalSet( const std::unique_ptr<SignalSet>& info ) {
    StpPhilipsReader::StpMetadata meta;
    if ( info->metadata( ).count( "Patient Name" ) > 0 ) {
      meta.name = info->metadata( ).at( "Patient Name" );
    }
    if ( info->metadata( ).count( "MRN" ) > 0 ) {
      meta.mrn = info->metadata( ).at( "MRN" );
    }

    if ( info->offsets( ).size( ) > 0 ) {
      meta.segment_count = (int) ( info->offsets( ).size( ) );
      std::vector<dr_time> times;
      for ( auto en : info->offsets( ) ) {
        times.push_back( en.second );
      }
      std::sort( times.begin( ), times.end( ) );
      meta.start_utc = times[0];
      meta.stop_utc = times[info->offsets( ).size( ) - 1];
    }

    return meta;
  }
}