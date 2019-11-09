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
  const StpReader::BlockConfig StpReader::HR = BlockConfig::vital( "HR", "Bpm" );
  const StpReader::BlockConfig StpReader::PVC = BlockConfig::vital( "PVC", "Bpm" );
  const StpReader::BlockConfig StpReader::STI = BlockConfig::div10( "ST-I", "mm", 1, false );
  const StpReader::BlockConfig StpReader::STII = BlockConfig::div10( "ST-II", "mm", 1, false );
  const StpReader::BlockConfig StpReader::STIII = BlockConfig::div10( "ST-III", "mm", 1, false );
  const StpReader::BlockConfig StpReader::STAVR = BlockConfig::div10( "ST-AVR", "mm", 1, false );
  const StpReader::BlockConfig StpReader::STAVL = BlockConfig::div10( "ST-AVL", "mm", 1, false );
  const StpReader::BlockConfig StpReader::STAVF = BlockConfig::div10( "ST-AVF", "mm", 1, false );
  const StpReader::BlockConfig StpReader::STV = BlockConfig::div10( "ST-V", "mm", 1, false );
  const StpReader::BlockConfig StpReader::BT = BlockConfig::div10( "BT", "Deg C", 2 );
  const StpReader::BlockConfig StpReader::IT = BlockConfig::div10( "IT", "Deg C", 2 );
  const StpReader::BlockConfig StpReader::RESP = BlockConfig::vital( "RESP", "BrMin" );
  const StpReader::BlockConfig StpReader::APNEA = BlockConfig::vital( "APNEA", "mmHg" );
  const StpReader::BlockConfig StpReader::NBP_M = BlockConfig::vital( "NBP-M", "mmHg" );
  const StpReader::BlockConfig StpReader::NBP_S = BlockConfig::vital( "NBP-S", "mmHg" );
  const StpReader::BlockConfig StpReader::NBP_D = BlockConfig::vital( "NBP-D", "mmHg" );
  const StpReader::BlockConfig StpReader::CUFF = BlockConfig::vital( "CUFF", "mmHg", 2, false );
  const StpReader::BlockConfig StpReader::AR1_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpReader::BlockConfig StpReader::AR1_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpReader::BlockConfig StpReader::AR1_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpReader::BlockConfig StpReader::AR1_R = BlockConfig::vital( "AR1-R", "mmHg" );
  const StpReader::BlockConfig StpReader::AR2_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpReader::BlockConfig StpReader::AR2_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpReader::BlockConfig StpReader::AR2_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpReader::BlockConfig StpReader::AR2_R = BlockConfig::vital( "AR1-R", "mmHg" );
  const StpReader::BlockConfig StpReader::AR3_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpReader::BlockConfig StpReader::AR3_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpReader::BlockConfig StpReader::AR3_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpReader::BlockConfig StpReader::AR3_R = BlockConfig::vital( "AR1-R", "mmHg" );
  const StpReader::BlockConfig StpReader::AR4_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpReader::BlockConfig StpReader::AR4_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpReader::BlockConfig StpReader::AR4_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpReader::BlockConfig StpReader::AR4_R = BlockConfig::vital( "AR1-R", "mmHg" );

  const StpReader::BlockConfig StpReader::SPO2_P = BlockConfig::vital( "SPO2-%", "%" );
  const StpReader::BlockConfig StpReader::SPO2_R = BlockConfig::vital( "SPO2-R", "Bpm" );
  const StpReader::BlockConfig StpReader::VENT = BlockConfig::vital( "Vent Rate", "BrMin" );
  const StpReader::BlockConfig StpReader::IN_HLD = BlockConfig::div10( "IN_HLD", "Sec", 2, false );
  const StpReader::BlockConfig StpReader::PRS_SUP = BlockConfig::vital( "PRS-SUP", "cmH2O" );
  const StpReader::BlockConfig StpReader::INSP_TM = BlockConfig::div10( "INSP-TM", "Sec", 2, false ); // FIXME: this is actually div 100!
  const StpReader::BlockConfig StpReader::INSP_PC = BlockConfig::vital( "INSP-PC", "%" );
  const StpReader::BlockConfig StpReader::I_E = BlockConfig::div10( "I:E", "", 2, false );
  const StpReader::BlockConfig StpReader::SET_PCP = BlockConfig::vital( "SET-PCP", "cmH2O" );
  const StpReader::BlockConfig StpReader::SET_IE = BlockConfig::div10( "SET-IE", "", 2, false );
  const StpReader::BlockConfig StpReader::APRV_LO_T = BlockConfig::div10( "APRV-LO-T", "Sec", 2, false );
  const StpReader::BlockConfig StpReader::APRV_HI_T = BlockConfig::div10( "APRV-HI-T", "Sec", 2, false );
  const StpReader::BlockConfig StpReader::APRV_LO = BlockConfig::vital( "APRV-LO", "cmH20", 2, false );
  const StpReader::BlockConfig StpReader::APRV_HI = BlockConfig::vital( "APRV-HI", "cmH20", 2, false );
  const StpReader::BlockConfig StpReader::RESIS = BlockConfig::div10( "RESIS", "cmH2O/L/Sec", 2, false );
  const StpReader::BlockConfig StpReader::MEAS_PEEP = BlockConfig::vital( "MEAS-PEEP", "cmH2O" );
  const StpReader::BlockConfig StpReader::INTR_PEEP = BlockConfig::vital( "INTR-PEEP", "cmH2O", 2, false );
  const StpReader::BlockConfig StpReader::INSP_TV = BlockConfig::vital( "INSP-TV", "" );
  const StpReader::BlockConfig StpReader::COMP = BlockConfig::vital( "COMP", "ml/cmH20" );

  const StpReader::BlockConfig StpReader::SPONT_MV = BlockConfig::vital( "SPONT-MV", "L/Min", 2, false );
  const StpReader::BlockConfig StpReader::SPONT_R = BlockConfig::vital( "SPONT-R", "BrMin", 2, false );
  const StpReader::BlockConfig StpReader::SET_TV = BlockConfig::vital( "SET-TV", "ml", 2, false );
  const StpReader::BlockConfig StpReader::B_FLW = BlockConfig::vital( "B-FLW", "L/Min", 2, false );
  const StpReader::BlockConfig StpReader::FLW_R = BlockConfig::vital( "FLW-R", "cmH20", 2, false );
  const StpReader::BlockConfig StpReader::FLW_TRIG = BlockConfig::vital( "FLW-TRIG", "L/Min", 2, false );
  const StpReader::BlockConfig StpReader::HF_FLW = BlockConfig::vital( "HF-FLW", "L/Min", 2, false );
  const StpReader::BlockConfig StpReader::HF_R = BlockConfig::vital( "HF-R", "Sec", 2, false );
  const StpReader::BlockConfig StpReader::HF_PRS = BlockConfig::vital( "HF-PRS", "cmH2O", 2, false );
  const StpReader::BlockConfig StpReader::TMP_1 = BlockConfig::div10( "TMP-1", "Deg C" );
  const StpReader::BlockConfig StpReader::TMP_2 = BlockConfig::div10( "TMP-2", "Deg C" );
  const StpReader::BlockConfig StpReader::DELTA_TMP = BlockConfig::div10( "DELTA-TMP", "Deg C" );
  const StpReader::BlockConfig StpReader::LA1 = BlockConfig::vital( "LA1", "mmHg" );
  const StpReader::BlockConfig StpReader::CVP1 = BlockConfig::vital( "CVP1", "mmHg" );
  const StpReader::BlockConfig StpReader::CPP1 = BlockConfig::vital( "CPP1", "mmHg" );
  const StpReader::BlockConfig StpReader::ICP1 = BlockConfig::vital( "ICP1", "mmHg" );
  const StpReader::BlockConfig StpReader::SP1 = BlockConfig::vital( "SP1", "mmHg" );
  const StpReader::BlockConfig StpReader::PA1_S = BlockConfig::vital( "PA1-S", "mmHg" );
  const StpReader::BlockConfig StpReader::PA1_D = BlockConfig::vital( "PA1-D", "mmHg" );
  const StpReader::BlockConfig StpReader::PA1_R = BlockConfig::vital( "PA1-R", "mmHg" );
  const StpReader::BlockConfig StpReader::PA1_M = BlockConfig::vital( "PA1-M", "mmHg" );
  const StpReader::BlockConfig StpReader::PA2_S = BlockConfig::vital( "PA2-S", "mmHg" );
  const StpReader::BlockConfig StpReader::PA2_D = BlockConfig::vital( "PA2-D", "mmHg" );
  const StpReader::BlockConfig StpReader::PA2_R = BlockConfig::vital( "PA2-R", "mmHg" );
  const StpReader::BlockConfig StpReader::PA2_M = BlockConfig::vital( "PA2-M", "mmHg" );
  const StpReader::BlockConfig StpReader::PA3_S = BlockConfig::vital( "PA3-S", "mmHg" );
  const StpReader::BlockConfig StpReader::PA3_D = BlockConfig::vital( "PA3-D", "mmHg" );
  const StpReader::BlockConfig StpReader::PA3_R = BlockConfig::vital( "PA3-R", "mmHg" );
  const StpReader::BlockConfig StpReader::PA3_M = BlockConfig::vital( "PA3-M", "mmHg" );
  const StpReader::BlockConfig StpReader::PA4_S = BlockConfig::vital( "PA4-S", "mmHg" );
  const StpReader::BlockConfig StpReader::PA4_D = BlockConfig::vital( "PA4-D", "mmHg" );
  const StpReader::BlockConfig StpReader::PA4_R = BlockConfig::vital( "PA4-R", "mmHg" );
  const StpReader::BlockConfig StpReader::PA4_M = BlockConfig::vital( "PA4-M", "mmHg" );

  const StpReader::BlockConfig StpReader::PT_RR = BlockConfig::vital( "PT-RR", "BrMin" );
  const StpReader::BlockConfig StpReader::PEEP = BlockConfig::vital( "PEEP", "cmH20" );
  const StpReader::BlockConfig StpReader::MV = BlockConfig::div10( "MV", "L/min" );
  const StpReader::BlockConfig StpReader::Fi02 = BlockConfig::vital( "Fi02", "%" );
  const StpReader::BlockConfig StpReader::TV = BlockConfig::vital( "TV", "ml" );
  const StpReader::BlockConfig StpReader::PIP = BlockConfig::vital( "PIP", "cmH20" );
  const StpReader::BlockConfig StpReader::PPLAT = BlockConfig::vital( "PPLAT", "cmH20" );
  const StpReader::BlockConfig StpReader::MAWP = BlockConfig::vital( "MAWP", "cmH20" );
  const StpReader::BlockConfig StpReader::SENS = BlockConfig::div10( "SENS", "cmH20" );

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


      for ( const auto& v : info->vitals( ) ) {
        while ( !v->empty( ) ) {
          auto dr = v->pop( );
          output( ) << v->name( ) << " " << dr->data << std::endl;
        }
      }
      info->reset( );


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
      size_t waveoffset = readUInt16( ) + 64;
      work.skip( 4 ); // don't know what these mean
      work.skip( 2 ); // don't know what these mean, either

      if ( 0x013A == readUInt16( ) ) {
        readDataBlock( info,{ SKIP2, HR, PVC, SKIP4, STI, STII, STIII, STV, SKIP5, STAVR, STAVL, STAVF }, 62 );

        if ( 0x013A != readUInt16( ) ) {
          // we expected a "closing" 0x013A, so something is wrong
          return ReadResult::ERROR;
        }
      }

      while ( work.readSinceMark( ) < waveoffset - 6 ) {
        work.skip( 66 );
        unsigned int blocktype = readUInt8( );
        unsigned int blockfmt = readUInt8( );
        work.rewind( 68 ); // go back to the start of this block
        output( ) << "new block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
            << blocktype << " " << blockfmt << " starting at " << std::dec << work.readSinceMark( ) << std::endl;
        //int combined = ( blocktype << 8 | blockfmt );
        switch ( blocktype ) {
          case 0x02:
            switch ( blockfmt ) {
              case 0x4D:
                readDataBlock( info,{ SKIP6, AR1_M, AR1_S, AR1_D, SKIP2, AR1_R } );
                break;
              case 0x4E:
                readDataBlock( info,{ SKIP6, AR2_M, AR2_S, AR2_D, SKIP2, AR2_R } );
                break;
              case 0x4F:
                readDataBlock( info,{ SKIP6, AR3_M, AR3_S, AR3_D, SKIP2, AR3_R } );
                break;
              case 0x50:
                readDataBlock( info,{ SKIP6, AR4_M, AR4_S, AR4_D, SKIP2, AR4_R } );
                break;
              default:
                readDataBlock( info,{ } );
                break;
            }
            break;
          case 0x03:
            switch ( blockfmt ) {
              case 0x4D:
                readDataBlock( info,{ SKIP6, PA1_M, PA1_S, PA1_D, SKIP2, PA1_R } );
                break;
              case 0x4E:
                readDataBlock( info,{ SKIP6, PA2_M, PA2_S, PA2_D, SKIP2, PA2_R } );
                break;
              case 0x4F:
                readDataBlock( info,{ SKIP6, PA3_M, PA3_S, PA3_D, SKIP2, PA3_R } );
                break;
              case 0x50:
                readDataBlock( info,{ SKIP6, PA4_M, PA4_S, PA4_D, SKIP2, PA4_R } );
                break;
              default:
                readDataBlock( info,{ } );
                break;
            }
            break;
          case 0x04:
            readDataBlock( info,{ SKIP6, LA1 } );
            break;
          case 0x05:
            readDataBlock( info,{ SKIP6, CVP1 } );
            break;
          case 0x06:
            if ( blockfmt == 0x4D ) {
              readDataBlock( info,{ SKIP6, ICP1, CPP1 } );
            }
            else {
              readDataBlock( info,{ } );
            }
            break;
          case 0x07:
            if ( blockfmt == 0x4D ) {
              readDataBlock( info,{ SKIP6, SP1 } );
            }
            else {
              readDataBlock( info,{ } );
            }
            break;
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
            readDataBlock( info,{ SKIP6, SPO2_P, SPO2_R } );
            break;
          case 0x0C:
            readDataBlock( info,{ SKIP6, TMP_1, TMP_2, DELTA_TMP } );
            break;
          case 0x0D:
            readDataBlock( info,{ } );
            break;
          case 0x13:
            readDataBlock( info,{ } );
            break;
          case 0x14:
            readDataBlock( info,{ SKIP6, PT_RR, PEEP, MV, SKIP2, Fi02, TV, PIP, PPLAT, MAWP, SENS } );
            break;
          case 0x2A:
            switch ( blockfmt ) {
              case 0xDB:
                readDataBlock( info,{ SKIP6, VENT, FLW_R, SKIP4, IN_HLD, SKIP2, PRS_SUP, INSP_TM, INSP_PC, I_E } );
                break;
              case 0xDC:
                readDataBlock( info,{ SKIP6, HF_FLW, HF_R, HF_PRS, SPONT_MV, SKIP2, SET_TV, SET_PCP, SET_IE, B_FLW, FLW_TRIG } );
                break;
              case 0x5C:
                readDataBlock( info,{ SKIP6, APRV_LO, APRV_HI, APRV_LO_T, SKIP2, APRV_HI_T, COMP, RESIS, MEAS_PEEP, INTR_PEEP, SPONT_R } );
                break;
              case 0x5D:
                readDataBlock( info,{ SKIP6, INSP_TV } );
                break;
              default:
                readDataBlock( info,{ } );
                break;
            }
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

  unsigned int StpReader::readUInt16( ) {
    unsigned char b1 = work.pop( );
    unsigned char b2 = work.pop( );
    return ( b1 << 8 | b2 );
  }

  int StpReader::readInt16( ) {
    char b1 = work.pop( );
    char b2 = work.pop( );
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

  void StpReader::readDataBlock( std::unique_ptr<SignalSet>& info, const std::vector<BlockConfig>& vitals, size_t blocksize ) {
    size_t read = 0;
    for ( const auto& cfg : vitals ) {
      read += cfg.readcount;
      if ( cfg.isskip ) {
        //output( ) << "skipping";
        //        for ( size_t i = 0; i < cfg.readcount; i++ ) {
        //          output( ) << " " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << (int) work.pop( );
        //        }
        //        output( ) << std::endl;
        work.skip( cfg.readcount );
      }
      else {
        bool okval = false;
        bool added = false;
        if ( cfg.unsign ) {
          unsigned int val;
          if ( 1 == cfg.readcount ) {
            val = readUInt8( );
            okval = ( val != 0x80 );
          }
          else {
            val = readUInt16( );
            okval = ( val != 0x8000 );
          }

          if ( okval ) {
            auto& sig = info->addVital( cfg.label, &added );
            if ( added ) {
              sig->setChunkIntervalAndSampleRate( 2000, 1 );
              sig->setUom( cfg.uom );
            }

            if ( cfg.divBy10 ) {
              sig->add( DataRow( currentTime, div10s( val ) ) );
            }
            else {
              sig->add( DataRow( currentTime, std::to_string( val ) ) );
            }
          }
        }
        else {
          int val;
          if ( 1 == cfg.readcount ) {
            val = readInt8( );
            okval = ( val > -128 ); // can't use hex values here, because our ints are signed
          }
          else {
            val = readInt16( );
            okval = ( val > -32767 );
          }

          if ( okval ) {
            auto& sig = info->addVital( cfg.label, &added );
            if ( added ) {
              sig->setChunkIntervalAndSampleRate( 2000, 1 );
              sig->setUom( cfg.uom );
            }

            if ( cfg.divBy10 ) {
              sig->add( DataRow( currentTime, div10s( val ) ) );
            }
            else {
              sig->add( DataRow( currentTime, std::to_string( val ) ) );
            }
          }
        }
      }
    }

    if ( 0 == read ) {
      output( ) << "  nothing specified for data block" << std::endl;
      read = 18;
      output( ) << "\tblock (" << std::dec << work.readSinceMark( ) << "):";
      for ( size_t i = 0; i < read; i++ ) {
        output( ) << " " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << readUInt8( );
      }
      output( ) << std::endl;
    }

    work.skip( blocksize - read );
  }

  std::string StpReader::div10s( int val ) {
    if ( 0 == val ) {
      return "0";
    }
    else if ( 0 == val % 10 ) {
      // if our number ends in 0, lop it off and return
      return std::to_string( val / 10 );
    }

    // else we're in the soup...
    // make sure our precision matches the number of positions in our number
    // which avoids rounding
    int prec = 2;
    if ( val >= 100 || val <= -100 ) {
      prec++;
    }
    if ( val >= 1000 || val <= -1000 ) {
      prec++;
      std::cout << val << std::endl;
    }
    if ( val >= 10000 || val <= -10000 ) {
      prec++;
      std::cout << val << std::endl;
    }

    std::stringstream ss;
    ss << std::dec << std::setprecision( prec ) << val / 10.0;
    std::string s;
    ss>>s;
    return s;
  }
}