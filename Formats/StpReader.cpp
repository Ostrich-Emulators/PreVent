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
#include "BasicSignalSet.h"

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

  const std::map<int, std::string> StpReader::WAVELABELS = {
    {0x07, "I" },
    {0x08, "II" },
    {0x09, "III" },
    {0x0A, "V" },
    {0x17, "RR" },
    {0x1B, "AR1" },
    {0x27, "SPO2" },
    {0xC8, "VNT_PRES" },
    {0xC9, "VNT_FLOW" },
  };

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
  const StpReader::BlockConfig StpReader::INSP_TM = BlockConfig::div100( "INSP-TM", "Sec", 2, false );
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
  const StpReader::BlockConfig StpReader::CVP2 = BlockConfig::vital( "CVP1", "mmHg" );
  const StpReader::BlockConfig StpReader::CVP3 = BlockConfig::vital( "CVP1", "mmHg" );
  const StpReader::BlockConfig StpReader::CVP4 = BlockConfig::vital( "CVP4", "mmHg" );
  const StpReader::BlockConfig StpReader::CPP1 = BlockConfig::vital( "CPP1", "mmHg" );
  const StpReader::BlockConfig StpReader::ICP1 = BlockConfig::vital( "ICP1", "mmHg" );
  const StpReader::BlockConfig StpReader::CPP2 = BlockConfig::vital( "CPP2", "mmHg" );
  const StpReader::BlockConfig StpReader::ICP2 = BlockConfig::vital( "ICP2", "mmHg" );
  const StpReader::BlockConfig StpReader::CPP3 = BlockConfig::vital( "CPP3", "mmHg" );
  const StpReader::BlockConfig StpReader::ICP3 = BlockConfig::vital( "ICP3", "mmHg" );
  const StpReader::BlockConfig StpReader::CPP4 = BlockConfig::vital( "CPP4", "mmHg" );
  const StpReader::BlockConfig StpReader::ICP4 = BlockConfig::vital( "ICP4", "mmHg" );
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

  const StpReader::BlockConfig StpReader::UAC1_S = BlockConfig::vital( "UAC1-S", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC1_D = BlockConfig::vital( "UAC1-D", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC1_R = BlockConfig::vital( "UAC1-R", "Bpm" );
  const StpReader::BlockConfig StpReader::UAC1_M = BlockConfig::vital( "UAC1-M", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC2_S = BlockConfig::vital( "UAC2-S", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC2_D = BlockConfig::vital( "UAC2-D", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC2_R = BlockConfig::vital( "UAC2-R", "Bpm" );
  const StpReader::BlockConfig StpReader::UAC2_M = BlockConfig::vital( "UAC2-M", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC3_S = BlockConfig::vital( "UAC3-S", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC3_D = BlockConfig::vital( "UAC3-D", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC3_R = BlockConfig::vital( "UAC3-R", "Bpm" );
  const StpReader::BlockConfig StpReader::UAC3_M = BlockConfig::vital( "UAC3-M", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC4_S = BlockConfig::vital( "UAC4-S", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC4_D = BlockConfig::vital( "UAC4-D", "mmHg" );
  const StpReader::BlockConfig StpReader::UAC4_R = BlockConfig::vital( "UAC4-R", "Bpm" );
  const StpReader::BlockConfig StpReader::UAC4_M = BlockConfig::vital( "UAC4-M", "mmHg" );

  const StpReader::BlockConfig StpReader::PT_RR = BlockConfig::vital( "PT-RR", "BrMin" );
  const StpReader::BlockConfig StpReader::PEEP = BlockConfig::vital( "PEEP", "cmH20" );
  const StpReader::BlockConfig StpReader::MV = BlockConfig::div10( "MV", "L/min" );
  const StpReader::BlockConfig StpReader::Fi02 = BlockConfig::vital( "Fi02", "%" );
  const StpReader::BlockConfig StpReader::TV = BlockConfig::vital( "TV", "ml" );
  const StpReader::BlockConfig StpReader::PIP = BlockConfig::vital( "PIP", "cmH20" );
  const StpReader::BlockConfig StpReader::PPLAT = BlockConfig::vital( "PPLAT", "cmH20" );
  const StpReader::BlockConfig StpReader::MAWP = BlockConfig::vital( "MAWP", "cmH20" );
  const StpReader::BlockConfig StpReader::SENS = BlockConfig::div10( "SENS", "cmH20" );

  const StpReader::BlockConfig StpReader::CO2_EX = BlockConfig::vital( "CO2-EX", "mmHg" );
  const StpReader::BlockConfig StpReader::CO2_IN = BlockConfig::vital( "CO2-IN", "mmHg" );
  const StpReader::BlockConfig StpReader::CO2_RR = BlockConfig::vital( "CO2-RR", "BrMin" );
  const StpReader::BlockConfig StpReader::O2_EXP = BlockConfig::div10( "O2-EXP", "%" );
  const StpReader::BlockConfig StpReader::O2_INSP = BlockConfig::div10( "O2-INSP", "%" );

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

    //    zstr::ifstream doublereader( filename );
    //    char * skipper = new char[1024 * 1024];
    //    std::ofstream outy( "uncompressed.segs" );
    //    do {
    //      doublereader.read( skipper, 1024 * 1024 );
    //      for ( size_t i = 0; i < 1024 * 1024; i++ ) {
    //        outy << skipper[i];
    //      }
    //    }
    //    while ( doublereader.gcount( ) > 0 );
    //    outy.close( );
    //    delete []skipper;

    filestream = new zstr::ifstream( filename );
    decodebuffer.reserve( 1024 * 32 );
    magiclong = 0;
    return 0;
  }

  ReadResult StpReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {
    output( ) << "initial reading from input stream" << std::endl;
    filestream->read( (char*) ( &decodebuffer[0] ), decodebuffer.capacity( ) );
    std::streamsize cnt = filestream->gcount( );
    output( ) << "work buffer capacity: " << work.capacity( ) << std::endl;

    while ( 0 != cnt ) {
      if ( work.available( ) < 1024 * 768 ) {
        // we should never come close to filling up our work buffer
        // so if we have, make sure the sure knows
        std::cerr << "work buffer is too full...something is going wrong" << std::endl;
        return ReadResult::ERROR;
      }

      //      output( ) << "read " << cnt << " bytes from input" << std::endl
      //          << "size before: " << work.size( ) << std::endl;
      // put everything on our buffer, and we'll read only full segments from there
      for ( int i = 0; i < cnt; i++ ) {
        work.push( decodebuffer[i] );
      }
      //      output( ) << "size after : " << work.size( ) << std::endl
      //          << "     avail : " << work.available( ) << std::endl;

      if ( ReadResult::FIRST_READ == lastrr && 0 == magiclong ) {
        // the first 4 bytes of the file are the segment demarcator
        magiclong = popUInt64( );
        work.rewind( 4 );
      }

      ChunkReadResult rslt = ChunkReadResult::OK;
      // read as many segments as we can before reading more data
      size_t segsize = 0;
      while ( workHasFullSegment( &segsize ) && ChunkReadResult::OK == rslt ) {
        //output( ) << "next segment is " << segsize << " bytes big" << std::endl;

        size_t startpop = work.popped( );
        rslt = processOneChunk( info, segsize );
        size_t endpop = work.popped( );

        if ( ChunkReadResult::OK == rslt ) {
          size_t bytesread = endpop - startpop;
          //output( ) << "read " << bytesread << " bytes of segment" << std::endl;
          if ( bytesread < segsize ) {
            work.skip( segsize - bytesread );
            output( ) << "skipping ahead " << ( segsize - bytesread ) << " bytes to next segment at " << ( startpop + segsize ) << std::endl;
          }
          //copySaved( filler, info );
        }
        else {
          // something happened so rewind our to our mark
          output( ) << "rewinding to start of segment" << std::endl;
          work.rewindToMark( );

          return ( ChunkReadResult::ROLLOVER == rslt
              ? ReadResult::END_OF_DAY
              : ChunkReadResult::NEW_PATIENT == rslt
              ? ReadResult::END_OF_PATIENT
              : ReadResult::ERROR );
        }
      }

      filestream->read( (char*) ( &decodebuffer[0] ), decodebuffer.capacity( ) );
      cnt = filestream->gcount( );
    }

    output( ) << "file is exhausted" << std::endl;

    // copy any data we have left in our filler set to the real set


    // if we still have stuff in our work buffer, process it
    if ( !work.empty( ) ) {
      output( ) << "still have stuff in our work buffer!" << std::endl;
      processOneChunk( info, work.size( ) );
    }
    //copySaved( filler, info );
    return ReadResult::END_OF_FILE;
  }

  void StpReader::copySaved( std::unique_ptr<SignalSet>& from, std::unique_ptr<SignalSet>& to ) {
    // FIXME: use up our leftover waveforms at the current time
    //    output( ) << "copying temp data to real signalset" << std::endl;
    //    for ( auto e : from->metadata( ) ) {
    //      output( ) << e.first << " " << e.second << std::endl;
    //    }

    to->setMetadataFrom( *from );

    for ( auto& v : from->vitals( ) ) {
      auto& real = to->addVital( v->name( ) );
      real->setMetadataFrom( *v );
      while ( !v->empty( ) ) {
        auto dr = v->pop( );
        real->add( *dr );
      }
    }

    for ( auto& w : from->waves( ) ) {
      auto& real = to->addWave( w->name( ) );
      real->setMetadataFrom( *w );
      while ( !w->empty( ) ) {
        auto dr = w->pop( );
        real->add( *dr );
      }
    }
  }

  StpReader::ChunkReadResult StpReader::processOneChunk( std::unique_ptr<SignalSet>& info,
      const size_t& maxread ) {
    // we are guaranteed to have a complete segment in the work buffer
    // and the work buffer head is pointing to the start of the segment
    work.mark( );
    output( ) << "processing one chunk from byte " << work.popped( ) << std::endl;
    try {
      work.skip( 18 );
      dr_time oldtime = currentTime;
      currentTime = popTime( );
      if ( isRollover( oldtime, currentTime ) ) {
        return ChunkReadResult::ROLLOVER;
      }

      work.skip( 2 );
      std::string patient = popString( 32 );
      if ( !patient.empty( ) ) {
        if ( 0 == info->metadata( ).count( "Patient Name" ) ) {
          info->setMeta( "Patient Name", patient );
        }
        else if ( info->metadata( ).at( "Patient Name" ) != patient ) {
          return ChunkReadResult::NEW_PATIENT;
        }
      }
      work.skip( 2 );
      // offset is number of bytes from byte 64, but we want to track bytes
      // since we started reading (set our mark)
      size_t waveoffset = popUInt16( ) + 60; // offset is at pos 60 in the segment
      work.skip( 4 ); // don't know what these mean
      work.skip( 2 ); // don't know what these mean, either

      // We have two types of blocks here: the 0x013A block, which is 62 bytes
      // big and contains HR, PVC, and ST-* vitals, and all other blocks, which
      // are 68 (but sometimes 66?!) bytes big and contain everything else. Both
      // types are optional, but if the 0x013A block is present, it always seems
      // to be first. Our strategy is to keep looping until, if we do one more
      // loop, we'll pass our wave offset limit

      while ( work.poppedSinceMark( ) + 66 <= waveoffset ) {
        //output( ) << "psm: " << work.poppedSinceMark( ) << "\t" << work.popped( ) << std::endl;
        if ( 0x013A == readUInt16( ) ) {
          work.skip( 2 ); // the int16 we just read
          readDataBlock( info,{ SKIP2, HR, PVC, SKIP4, STI, STII, STIII, STV, SKIP5, STAVR, STAVL, STAVF }, 62 );

          if ( 0x013A != popUInt16( ) ) {
            // we expected a "closing" 0x013A, so something is wrong
            return ChunkReadResult::HR_BLOCK_PROBLEM;
          }
        }
        else {
          work.skip( 66 ); // skip to end of the block to read the block type and format
          unsigned int blocktype = popUInt8( );
          unsigned int blockfmt = popUInt8( );
          work.rewind( 68 ); // go back to the start of this block
          //output( ) << "new block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
          //  << blocktype << " " << blockfmt << " starting at " << std::dec << work.popped( ) << std::endl;
          switch ( blocktype ) {
            case 0x01:
              if ( 0x00 == blockfmt ) {
                // sometimes our 68-byte block is only 66 bytes big! Luckily,
                // this only seems to happen when the blocktype is actuall 0x0D,
                // which we ignore anyway. It seems to be followed by 0x0100,
                // so just ignore this, too
                readDataBlock( info,{ } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
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
                  unhandledBlockType( blocktype, blockfmt );
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
                  unhandledBlockType( blocktype, blockfmt );
                  break;
              }
              break;
            case 0x04:
              if ( 0x4D == blockfmt ) {
                readDataBlock( info,{ SKIP6, LA1 } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x05:
              if ( 0x4D == blockfmt ) {
                readDataBlock( info,{ SKIP6, CVP1 } );
              }
              else if ( 0x4E == blockfmt ) {
                readDataBlock( info,{ SKIP6, CVP2 } );
              }
              else if ( 0x4F == blockfmt ) {
                readDataBlock( info,{ SKIP6, CVP3 } );
              }
              else if ( 0x50 == blockfmt ) {
                readDataBlock( info,{ SKIP6, CVP4 } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x06:
              if ( 0x4D == blockfmt ) {
                readDataBlock( info,{ SKIP6, ICP1, CPP1 } );
              }
              else if ( 0x4E == blockfmt ) {
                readDataBlock( info,{ SKIP6, ICP2, CPP2 } );
              }
              else if ( 0x4F == blockfmt ) {
                readDataBlock( info,{ SKIP6, ICP3, CPP3 } );
              }
              else if ( 0x50 == blockfmt ) {
                readDataBlock( info,{ SKIP6, ICP4, CPP4 } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x07:
              if ( 0x4D == blockfmt ) {
                readDataBlock( info,{ SKIP6, SP1 } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x08:
              if ( 0x22 == blockfmt ) {
                readDataBlock( info,{ SKIP6, RESP, APNEA } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }

              break;
            case 0x09:
              if ( 0x22 == blockfmt ) {
                readDataBlock( info,{ SKIP6, BT, IT } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x0A:
              if ( 0x18 == blockfmt ) {
                readDataBlock( info,{ SKIP6, NBP_M, NBP_S, NBP_D, SKIP2, CUFF } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x0B:
              if ( 0x2D == blockfmt ) {
                readDataBlock( info,{ SKIP6, SPO2_P, SPO2_R } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x0C:
              if ( 0x22 == blockfmt || 0x23 == blockfmt ) {
                readDataBlock( info,{ SKIP6, TMP_1, TMP_2, DELTA_TMP } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x0D:
              readDataBlock( info,{ } );
              //unhandledBlockType( blocktype, blockfmt );
              break;

            case 0x0E:
              if ( blockfmt == 0x4D ) {
                readDataBlock( info,{ SKIP6, CO2_EX, CO2_IN, CO2_RR, SKIP2, O2_EXP, O2_INSP } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x10:
              if ( blockfmt == 0x4D ) {
                readDataBlock( info,{ SKIP6, UAC1_M, UAC1_S, UAC1_M, SKIP2, UAC1_R } );
              }
              else if ( 0x4E == blockfmt ) {
                readDataBlock( info,{ SKIP6, UAC2_M, UAC2_S, UAC2_M, SKIP2, UAC2_R } );
              }
              else if ( 0x4F == blockfmt ) {
                readDataBlock( info,{ SKIP6, UAC3_M, UAC3_S, UAC3_M, SKIP2, UAC3_R } );
              }
              else if ( 0x50 == blockfmt ) {
                readDataBlock( info,{ SKIP6, UAC4_M, UAC4_S, UAC4_M, SKIP2, UAC4_R } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
            case 0x14:
              if ( 0xC2 == blockfmt ) {
                readDataBlock( info,{ SKIP6, PT_RR, PEEP, MV, SKIP2, Fi02, TV, PIP, PPLAT, MAWP, SENS } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
              break;
            case 0x1D:
              if ( 0x0A == blockfmt ) {
                readDataBlock( info,{ SKIP6, NBP_M, NBP_S, NBP_D, SKIP2, CUFF } );
              }
              else {
                unhandledBlockType( blocktype, blockfmt );
              }
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
                  unhandledBlockType( blocktype, blockfmt );
                  break;
              }
              break;
            default:
              unhandledBlockType( blocktype, blockfmt );
          }
        }
      }

      if ( work.poppedSinceMark( ) < waveoffset ) {
        work.skip( waveoffset - work.poppedSinceMark( ) );
      }
      else if ( work.poppedSinceMark( ) > waveoffset ) {
        std::cerr << "we passed the wave start. that ain't right!" << std::endl;
        work.rewind( work.poppedSinceMark( ) - waveoffset );
      }

      // waves are always ok (?)
      //ChunkReadResult rslt = readWavesBlock( info );
      readWavesBlock( info, maxread );
      return ChunkReadResult::OK;
    }
    catch ( const std::runtime_error & err ) {
      std::cerr << err.what( ) << std::endl;
      return ChunkReadResult::UNKNOWN_BLOCKTYPE;
    }
  }

  void StpReader::unhandledBlockType( unsigned int type, unsigned int fmt ) const {
    std::stringstream ss;
    ss << "unhandled block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
        << type << " " << fmt << " starting at " << std::dec << work.popped( );
    throw std::runtime_error( ss.str( ) );
  }

  unsigned long StpReader::popUInt64( ) {
    std::vector<unsigned char> vec = work.popvec( 4 );
    return ( vec[0] << 24 | vec[1] << 16 | vec[2] << 8 | vec[3] );
  }

  unsigned int StpReader::popUInt16( ) {
    unsigned char b1 = work.pop( );
    unsigned char b2 = work.pop( );
    return ( b1 << 8 | b2 );
  }

  unsigned int StpReader::readUInt16( ) {
    unsigned char b1 = work.read( );
    unsigned char b2 = work.read( 1 );
    return ( b1 << 8 | b2 );
  }

  int StpReader::popInt16( ) {
    unsigned char b1 = work.pop( );
    unsigned char b2 = work.pop( );

    short val = ( b1 << 8 | b2 );
    return val;
  }

  int StpReader::popInt8( ) {
    return (char) work.pop( );
  }

  unsigned int StpReader::popUInt8( ) {
    return work.pop( );
  }

  std::string StpReader::popString( size_t length ) {
    std::vector<char> chars;
    chars.reserve( length );
    auto data = work.popvec( length );
    for ( size_t i = 0; i < data.size( ) && 0 != data[i]; i++ ) {
      chars.push_back( (char) data[i] );
    }
    return std::string( chars.begin( ), chars.end( ) );
  }

  dr_time StpReader::popTime( ) {
    // time is in little-endian format
    auto shorts = work.popvec( 4 );
    time_t time = ( ( shorts[1] << 24 ) | ( shorts[0] << 16 ) | ( shorts[3] << 8 ) | shorts[2] );
    return time * 1000;
  }

  StpReader::ChunkReadResult StpReader::readWavesBlock( std::unique_ptr<SignalSet>& info, const size_t& maxread ) {
    if ( 0x04 == work.read( ) ) {
      work.skip( 4 );
    }

    // if our wave section starts with an FA 0D (i.e., no wave data is present),
    // skip ahead to the next segment
    if ( 0xFA == work.read( ) && 0x0D == work.read( 1 ) ) {
      output( ) << "no wave data in waves section" << std::endl;
      return ChunkReadResult::OK;
    }

    output( ) << "waves section starts at: " << std::dec << work.popped( ) << std::endl;

    static const unsigned int READCOUNTS[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    std::map<int, unsigned int> expectedValues;
    std::map<int, std::vector<int>> wavevals;

    int fa0dloops = 0;
    // we know we have at least one full segment in our work buffer,
    // so keep reading until we hit the next segment...then write the appropriate number of values to the signalset
    // and keep any overrun for the next loop
    while ( 0x7E != work.read( ) && work.poppedSinceMark( ) < maxread ) {
      const unsigned int waveid = popUInt8( );
      const unsigned int countbyte = popUInt8( );

      // usually, we get 0x0B, but sometimes we get 0x3B. I don't know what
      // that means, but the B part seems to be the only thing that matters
      // so zero out the most significant bits
      unsigned int shifty = ( countbyte & 0b00000111 );
      unsigned int valstoread = READCOUNTS[shifty - 1];
      //      output( ) << "wave "
      //          << std::setfill( '0' ) << std::setw( 2 ) << std::hex << waveid << "; count:"
      //          << std::setfill( '0' ) << std::setw( 2 ) << std::hex << countbyte
      //          << "; shift code is: "
      //          << std::dec << shifty
      //          << "..." << std::dec << valstoread << " vals to read" << std::endl;

      if ( 0xFA == waveid && 0x0D == countbyte ) {
        fa0dloops++;
        //output( ) << "FA 0D section at byte: " << std::dec << ( work.popped( ) - 2 ) << " (loop " << fa0dloops << ")" << std::endl;
        // skip through this section to get to the next waveform section

        work.skip( 33 );
        //work.rewind( 33 );
        //        auto vec = work.popvec( 33 );
        //        for ( auto& i : vec ) {
        //          output( ) << "  " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << (unsigned int) i;
        //        }
        //        output( ) << std::endl;


        if ( 0x04 == work.read( ) ) {
          work.skip( 4 );
        }
        //        output( ) << "  wave counts:";
        //        for ( auto& e : wavevals ) {
        //          output( ) << "\t" << e.first << "(" << e.second.size( ) << ")";
        //        }
        //        output( ) << std::endl;
      }
      else if ( valstoread > 4 ) {
        std::stringstream ss;
        ss << "don't really think we want to read " << valstoread << " values for wave/count:"
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << waveid << " "
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << countbyte
            << " starting at " << std::dec << work.popped( );
        std::string ex = ss.str( );
        throw std::runtime_error( ex );
      }
      else if ( 0 == StpReader::WAVELABELS.count( waveid ) ) {
        std::stringstream ss;
        ss << "unknown wave id/count: "
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << waveid << " "
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << countbyte
            << " starting at " << std::dec << work.popped( );
        std::string ex = ss.str( );
        throw std::runtime_error( ex );
      }
      else {
        //        output( ) << "reading " << valstoread
        //            << " values starting at " << std::dec << work.readSinceMark( ) << std::endl;
        //if ( countbyte > 0x0C ) {
        // we had a 0x3B or something
        //          output( ) << "countbyte discrepancy! "
        //              << std::setfill( '0' ) << std::setw( 2 ) << std::hex << countbyte
        //              << " at byte " << std::dec << ( work.popped( ) - 1 ) << std::endl;
        //}

        if ( 0 == expectedValues.count( waveid ) ) {
          expectedValues[waveid] = valstoread * 120;
          wavevals[waveid].reserve( expectedValues[waveid] );

          // if we've already seen a couple fa0d loops,
          // fill in the missing values to this point
          if ( fa0dloops > 0 ) {
            // we expect 15 values per fa0d loop
            for ( unsigned int i = 0; i < fa0dloops * 15 * valstoread; i++ ) {
              wavevals[waveid].push_back( SignalData::MISSING_VALUE );
            }
          }
        }

        for ( size_t i = 0; i < valstoread; i++ ) {
          int inty = popInt16( );
          wavevals[waveid].push_back( inty );
        }
      }
    }

    // we read a whole waveform section, so we can add our values to the ones
    // we saved from previous loops, and then write one complete block
    for ( auto& w : wavevals ) {
      std::stringstream vals;
      std::vector<int>& vector = leftoverwaves[w.first];
      vector.insert( vector.end( ), w.second.begin( ), w.second.end( ) );

      if ( vector.size( ) != expectedValues[w.first] ) {
        output( ) << "wave " << w.first << " read " << w.second.size( )
            << " values for a total of " << vector.size( )
            << " expecting to write " << expectedValues[w.first] << " values..." << std::endl;

        int missingcount = expectedValues[w.first] - vector.size( );
        if ( missingcount > 0 ) {
          output( ) << "filling in " << missingcount << " missing values for wave wave " << w.first << std::endl;
          for ( int i = 0; i < missingcount; i++ ) {
            vector.push_back( SignalData::MISSING_VALUE );
          }
        }
      }

      // make sure we have at least one val above the error code limit (-32753)
      // while converting their values to a big string
      bool waveok = false;
      for ( size_t i = 0; i < expectedValues[w.first]; i++ ) {
        if ( vector[i]> -32753 ) {
          waveok = true;
        }
        if ( 0 != i ) {
          vals << ",";
        }
        vals << vector[i];
      }

      if ( waveok ) {
        //output( ) << vals.str( ) << std::endl;
        bool first = false;
        auto& signal = info->addWave( WAVELABELS.at( w.first ), &first );
        if ( first ) {
          signal->setChunkIntervalAndSampleRate( 2000, expectedValues[w.first] );
        }

        signal->add( DataRow( currentTime, vals.str( ) ) );
      }
      //      else {
      //        output( ) << "(wave not ok)";
      //      }
      vector.erase( vector.begin( ), vector.begin( ) + expectedValues[w.first] );
      //      output( ) << "after writing, " << vector.size( ) << " vals left for next loop" << std::endl;
      if ( !vector.empty( ) ) {
        output( ) << "keeping " << vector.size( ) << " values for next loop" << std::endl;
      }
    }

    return ChunkReadResult::OK;
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
            val = popUInt8( );
            okval = ( val != 0x80 );
          }
          else {
            val = popUInt16( );
            okval = ( val != 0x8000 );
          }

          if ( okval ) {
            auto& sig = info->addVital( cfg.label, &added );
            if ( added ) {
              sig->setChunkIntervalAndSampleRate( 2000, 1 );
              sig->setUom( cfg.uom );
            }

            if ( cfg.divBy10 ) {
              sig->add( DataRow( currentTime, div10s( val, cfg.divBy10 ) ) );
            }
            else {
              sig->add( DataRow( currentTime, std::to_string( val ) ) );
            }
          }
        }
        else {
          int val;
          if ( 1 == cfg.readcount ) {
            val = popInt8( );
            okval = ( val > -128 ); // can't use hex values here, because our ints are signed
          }
          else {
            val = popInt16( );
            okval = ( val > -32767 );
          }

          if ( okval ) {
            auto& sig = info->addVital( cfg.label, &added );
            if ( added ) {
              sig->setChunkIntervalAndSampleRate( 2000, 1 );
              sig->setUom( cfg.uom );
            }

            if ( cfg.divBy10 ) {
              sig->add( DataRow( currentTime, div10s( val, cfg.divBy10 ) ) );
            }
            else {

              sig->add( DataRow( currentTime, std::to_string( val ) ) );
            }
          }
        }
      }
    }

    //    if ( 0 == read ) {
    //      output( ) << "  no reads specified for block " << std::dec << work.popped( ) << ":\t";
    //      read = 18;
    //      for ( size_t i = 0; i < read; i++ ) {
    //        output( ) << " " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << popUInt8( );
    //      }
    //      output( ) << std::endl;
    //    }

    work.skip( blocksize - read );
  }

  std::string StpReader::div10s( int val, unsigned int multiple ) {
    if ( 0 == val ) {
      return "0";
    }

    int denoms[] = { 1, 10, 100, 1000, 10000 };
    int denominator = denoms[multiple];

    if ( 0 == val % denominator ) {
      // if our number ends in 0, lop it off and return
      return std::to_string( val / denominator );
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
    }
    if ( val >= 10000 || val <= -10000 ) {
      prec++;
    }

    std::stringstream ss;
    ss << std::dec << std::setprecision( prec ) << val / (double) denominator;

    return ss.str( );
  }

  bool StpReader::workHasFullSegment( size_t* size ) {
    // ensure the data in our buffer starts with a segment marker, and goes at
    // least to the next segment marker. we do this by reading each byte (sorry)

    if ( work.empty( ) ) {
      return false;
    }

    bool ok = false;
    try {
      work.mark( );
      // a segment starts with a 64 bit long, then two blank bytes, then
      // the long again, and then two more blank bytes... we'll just treat
      // it as a 6-byte string that is repeated. I believe the segment
      // always starts with 0x7E.

      // WARNING: if we don't start with our magic number, what do we do?
      // subsequent calls will never match

      std::string magic1 = popString( 6 );
      std::string magic2 = popString( 6 );
      if ( magic1 == magic2 ) {
        // segment started...now search for a 0x7E, and then check again

        // the two while loops are a little strange, but we only want to do
        // a comparison if we have a shot at getting a match
        unsigned long check = 0;
        while ( check != magiclong ) {
          while ( 0x7E != work.pop( ) ) {
            // skip forward until we see another 0x7E
          }
          work.rewind( ); // get that 0x7E back in the buffer
          check = popUInt64( );
        }
        ok = true;

        if ( nullptr != size ) {
          *size = ( work.poppedSinceMark( ) - 4 );
        }
      }
      else {
        ok = false;
      }
    }
    catch ( const std::runtime_error& x ) {
      // don't care, but don't throw
    }

    work.rewindToMark( );
    return ok;
  }
}