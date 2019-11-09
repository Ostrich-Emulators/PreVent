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

#include "StpReader.h"
#include "SignalData.h"
#include "DataRow.h"
#include "Hdf5Writer.h"
#include "StreamChunkReader.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <experimental/filesystem>
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

  const StpReader::BlockConfig StpReader::SKIP = BlockConfig::skip( );
  const StpReader::BlockConfig StpReader::SKIP2 = BlockConfig::skip( 2 );
  const StpReader::BlockConfig StpReader::SKIP4 = BlockConfig::skip( 4 );
  const StpReader::BlockConfig StpReader::SKIP5 = BlockConfig::skip( 5 );
  const StpReader::BlockConfig StpReader::SKIP6 = BlockConfig::skip( 6 );
  const StpReader::BlockConfig StpReader::HR = BlockConfig::vital( "HR" );
  const StpReader::BlockConfig StpReader::PVC = BlockConfig::vital( "PVC" );
  const StpReader::BlockConfig StpReader::STI = BlockConfig::vital( "ST-I", 1, true );
  const StpReader::BlockConfig StpReader::STII = BlockConfig::vital( "ST-II", 1, true );
  const StpReader::BlockConfig StpReader::STIII = BlockConfig::vital( "ST-III", 1, true );
  const StpReader::BlockConfig StpReader::STAVR = BlockConfig::vital( "ST-AVR", 1, true );
  const StpReader::BlockConfig StpReader::STAVL = BlockConfig::vital( "ST-AVL", 1, true );
  const StpReader::BlockConfig StpReader::STAVF = BlockConfig::vital( "ST-AVF", 1, true );
  const StpReader::BlockConfig StpReader::STV = BlockConfig::vital( "ST-V", 1, true );
  const StpReader::BlockConfig StpReader::BT = BlockConfig::vital( "BT", 2, true );
  const StpReader::BlockConfig StpReader::IT = BlockConfig::vital( "IT", 2, true );
  const StpReader::BlockConfig StpReader::RESP = BlockConfig::vital( "RESP" );
  const StpReader::BlockConfig StpReader::APNEA = BlockConfig::vital( "APNEA" );
  const StpReader::BlockConfig StpReader::NBP_M = BlockConfig::vital( "NBP-M" );
  const StpReader::BlockConfig StpReader::NBP_S = BlockConfig::vital( "NBP-S" );
  const StpReader::BlockConfig StpReader::NBP_D = BlockConfig::vital( "NBP-D" );
  const StpReader::BlockConfig StpReader::CUFF = BlockConfig::vital( "CUFF" );
  const StpReader::BlockConfig StpReader::AR1_M = BlockConfig::vital( "AR1-M" );
  const StpReader::BlockConfig StpReader::AR1_S = BlockConfig::vital( "AR1-S" );
  const StpReader::BlockConfig StpReader::AR1_D = BlockConfig::vital( "AR1-D" );
  const StpReader::BlockConfig StpReader::AR1_R = BlockConfig::vital( "AR1-R" );

  StpReader::StpReader( ) : Reader( "STP" ), firstread( true ), work( 1024 * 1024 ) {
  }

  StpReader::StpReader( const std::string& name ) : Reader( name ), firstread( true ), work( 1024 * 1024 ) {
  }

  StpReader::StpReader( const StpReader& orig ) : Reader( orig ), firstread( orig.firstread ), work( orig.work.capacity( ) ) {
  }

  StpReader::~StpReader( ) {
  }

  void StpReader::finish( ) {
    if ( filestream ) {
      delete filestream;
    }
  }

  int StpReader::prepare( const std::string& filename, std::unique_ptr<SignalSet>& data ) {
    int rslt = Reader::prepare( filename, data );
    if ( rslt != 0 ) {
      return rslt;
    }

    // STP files are a concatenation of zlib-compressed segments, so we need to
    // know when we're *really* done reading. Otherwise, the decoder will say
    // it's at the end of the file once the first segment is fully read
    struct stat filestat;
    if ( stat( filename.c_str( ), &filestat ) == -1 ) {
      perror( "stat failed" );
      return -1;
    }

    filestream = new zstr::ifstream( filename, std::ios::binary );
    decodebuffer.reserve( 1024 * 16 );
    return 0;
  }

  ReadResult StpReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {

    filestream->read( (char*) ( &decodebuffer[0] ), decodebuffer.capacity( ) );
    std::streamsize cnt = filestream->gcount( );
    while ( 0 != cnt ) {
      // put everything on our buffer, and we'll read only full segments from there
      for ( int i = 0; i < cnt; i++ ) {
        work.push( decodebuffer[i] );
      }

      ReadResult rslt = processOneChunk( info );
      while ( ReadResult::NORMAL == rslt ) {
        rslt = processOneChunk( info );
      }

      if ( ReadResult::NORMAL != rslt ) {
        // something happened so rewind our to our mark
        work.rewindToMark( );

        // ...but maybe we just have an incomplete segment
        // so reading more data will fix our problem
        if ( ReadResult::READER_DEPENDENT != rslt ) {
          // or not, so let the caller know
          return rslt;
        }
      }

      filestream->read( (char*) ( &decodebuffer[0] ), decodebuffer.capacity( ) );
      cnt = filestream->gcount( );
    }

    return ReadResult::END_OF_FILE;
  }

  ReadResult StpReader::processOneChunk( std::unique_ptr<SignalSet>& info ) {
    // NOTE: we don't know if our working data has a full segment in it,
    // so mark where we started reading, in case we need to rewind
    work.mark( );

    // make sure we're looking at a header block
    output( ) << "processing one chunk" << std::endl;
    if ( !( work.read( ) == 0x7E && work.read( 6 ) == 0x7E ) ) {
      // we're not looking at a header block, so something's wrong
      return ReadResult::ERROR;
    }

    try {
      work.skip( 18 );
      dr_time timer = readTime( );
      if ( isRollover( currentTime, timer ) ) {
        return ReadResult::END_OF_DAY;
      }
      currentTime = timer;

      work.skip( 2 );
      info->setMeta( "Patient Name", readString( 32 ) );
      work.skip( 2 );
      // offset is number of bytes from byte 64, but we want to track bytes
      // since we started reading (set our mark)
      size_t waveoffset = readInt16( ) + 64;
      work.skip( 4 ); // don't know what these mean
      work.skip( 2 ); // don't know what these mean, either

      if ( 0x013A == readInt16( ) ) {
        readHrBlock( info );
        if ( 0x013A != readInt16( ) ) {
          // we expected a "closing" 0x013A, so something is wrong
          return ReadResult::ERROR;
        }
      }

      while ( work.readSinceMark( ) < waveoffset - 6 ) {
        work.skip( 66 );
        int blocktype = readInt8( );
        int blockfmt = readInt8( );
        work.rewind( 68 ); // go back to the start of this block
        output( ) << "new block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
            << blocktype << " " << blockfmt << " starting at " << std::dec << work.readSinceMark( ) << std::endl;
        //int combined = ( blocktype << 8 | blockfmt );
        switch ( blocktype ) {
          case 0x02:
            readDataBlock( info,{ SKIP6, AR1_M, AR1_S, AR1_D, SKIP2, AR1_R } );
          case 0x08:
            readDataBlock( info,{ SKIP6, RESP, APNEA } );
            break;
          case 0x09:
            readDataBlock( info,{ SKIP6, BT, IT } );
            break;
          case 0x0A:
            readDataBlock( info,{ SKIP6, NBP_M, NBP_S, NBP_D, SKIP2, CUFF } );
            break;
          case 0x0B:
            readDataBlock( info,{ SKIP6, NBP_M, NBP_S, NBP_D, SKIP2, CUFF } );
            break;
          case 0x0D:
            readDataBlock( info,{ } );
            break;
        }
      }
      work.skip( 6 );
      output( ) << "first wave id: " << std::setfill( '0' ) << std::setw( 2 )
          << std::hex << readInt8( ) << "; vals:" << std::setfill( '0' ) << std::setw( 2 )
          << readInt8( ) << std::endl;

      return ReadResult::END_OF_PATIENT;
      //return ReadResult::NORMAL;
    }
    catch ( const std::runtime_error & err ) {
      work.rewindToMark( );
      // hopefully, we just need a little more data to read a full segment
      return ReadResult::READER_DEPENDENT;
    }
  }

  int StpReader::readInt16( ) {
    unsigned char b1 = work.pop( );
    unsigned char b2 = work.pop( );
    return ( b1 << 8 | b2 );
  }

  int StpReader::readInt8( ) {
    return (char) work.pop( );
  }

  unsigned int StpReader::readUInt8( ) {
    return work.pop( );
  }

  std::string StpReader::readString( size_t length ) {
    std::vector<char> chars;
    chars.reserve( length );
    auto data = work.popvec( length );
    for ( size_t i = 0; i < data.size( ); i++ ) {
      chars.push_back( (char) data[i] );
    }
    return std::string( chars.begin( ), chars.end( ) );
  }

  dr_time StpReader::readTime( ) {
    // time is in little-endian format
    auto shorts = work.popvec( 4 );
    time_t time = ( ( shorts[1] << 24 ) | ( shorts[0] << 16 ) | ( shorts[3] << 8 ) | shorts[2] );
    return time * 1000;
  }

  bool StpReader::waveIsOk( const std::string& wavedata ) {
    // if all the values are -32768 or -32753, this isn't a valid reading
    std::stringstream stream( wavedata );
    for ( std::string each; std::getline( stream, each, ',' ); ) {
      if ( !( "-32768" == each || "-32753" == each ) ) {
        return true;
      }
    }
    return false;
  }

  void StpReader::readDataBlock( std::unique_ptr<SignalSet>& info, const std::vector<BlockConfig>& vitals ) {
    size_t read = 0;
    for ( const auto& cfg : vitals ) {
      read += cfg.readcount;
      if ( cfg.isskip ) {
        output( ) << "skipping";
        for ( size_t i = 0; i < cfg.readcount; i++ ) {
          output( ) << " " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << (int) work.pop( );
        }
        output( ) << std::endl;
        //work.skip( cfg.readcount );
      }
      else {
        bool okval = false;
        int val;
        if ( 1 == cfg.readcount ) {
          val = readInt8( );
          okval = ( val != 0x80 );
        }
        else {
          val = readInt16( );
          okval = ( val != 0x8000 );
        }

        if ( okval ) {
          auto& sig = info->addVital( cfg.label );

          if ( cfg.divBy10 ) {
            sig->add( DataRow( currentTime, div10s( val ) ) );
          }
          else {
            sig->add( DataRow( currentTime, std::to_string( val ) ) );
          }
        }
      }
    }

    work.skip( 68 - read );
  }

  void StpReader::readHrBlock( std::unique_ptr<SignalSet>& info ) {
    work.skip( 2 );
    std::map<std::string, int> hrmap = {
      {"HR", readInt16( ) },
      {"PVC", readInt16( ) },
    };

    for ( auto& en : hrmap ) {
      //      output( ) << en.first << " " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
      //          << en.second << std::endl;
      if ( en.second != 0x8000 ) {
        auto& hrsig = info->addVital( en.first );
        hrsig->add( DataRow( currentTime, std::to_string( en.second ) ) );
      }
    }

    work.skip( 4 );
    std::map<std::string, int> stmap = {
      {"ST-I", readInt8( ) },
      {"ST-II", readInt8( ) },
      {"ST-III", readInt8( ) },
      {"ST-V", readInt8( ) },
    };
    for ( auto& en : stmap ) {
      //      output( ) << en.first << " " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
      //          << en.second << " " << std::dec << en.second << std::endl;
      if ( en.second != 0x80 ) {
        auto& sig = info->addVital( en.first );
        sig->add( DataRow( currentTime, div10s( en.second ) ) );
      }
    }

    work.skip( 5 );
    std::map<std::string, short> avmap = {
      {"ST-AVR", readInt8( ) },
      {"ST-AVL", readInt8( ) },
      {"ST-AVF", readInt8( ) },
    };
    for ( auto& en : avmap ) {
      //      output( ) << en.first << " " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
      //          << en.second << " " << std::dec << en.second << std::endl;
      if ( en.second != 0x80 ) {
        auto& sig = info->addVital( en.first );
        sig->add( DataRow( currentTime, div10s( en.second ) ) );
      }
    }

    work.skip( 40 );
  }

  static std::string StpReader::div10s( int val ) {
    if ( 0 == val ) {
      return "0";
    }
    std::stringstream ss;
    ss << std::dec << std::setprecision( 2 ) << val / 10.0;
    std::string s;
    ss>>s;
    return s;
  }
}