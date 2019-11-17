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
    decodebuffer.reserve( 1024 * 16 );
    return 0;
  }

  ReadResult StpReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {
    output( ) << "initial reading from input stream" << std::endl;
    filestream->read( (char*) ( &decodebuffer[0] ), decodebuffer.capacity( ) );
    std::streamsize cnt = filestream->gcount( );
    output( ) << "read " << cnt << " bytes from input" << std::endl;
    output( ) << "work buffer size : " << work.size( ) << std::endl;
    output( ) << "             cap : " << work.capacity( ) << std::endl;
    output( ) << "           avail : " << work.available( ) << std::endl;

    std::unique_ptr<SignalSet> filler( new BasicSignalSet( ) );

    while ( 0 != cnt ) {
      // put everything on our buffer, and we'll read only full segments from there
      for ( int i = 0; i < cnt; i++ ) {
        work.push( decodebuffer[i] );
      }

      if ( ReadResult::FIRST_READ == lastrr ) {
        // the first 4 bytes of the file are the segment demarcator
        std::vector<unsigned char> vec = work.popvec( 4 );
        magiclong = ( vec[0] << 24 | vec[1] << 16 | vec[2] << 8 | vec[3] );
        work.rewind( 4 );
      }

      ReadResult rslt;
      do {
        filler->reset( );

        size_t startpop = work.popped( );
        rslt = processOneChunk( filler );
        size_t endpop = work.popped( );
        if ( rslt == ReadResult::NORMAL ) {
          output( ) << "chunk was " << ( endpop - startpop ) << " bytes big" << std::endl;
          copySaved( filler, info );
        }
      }
      while ( ReadResult::NORMAL == rslt && !work.empty( ) );

      //      for ( const auto& v : info->vitals( ) ) {
      //        while ( !v->empty( ) ) {
      //          auto dr = v->pop( );
      //          output( ) << v->name( ) << " " << dr->data << std::endl;
      //        }
      //      }
      //      for ( const auto& v : info->waves( ) ) {
      //        while ( !v->empty( ) ) {
      //          auto dr = v->pop( );
      //          output( ) << v->name( ) << std::endl << "\t" << dr->data << std::endl;
      //        }
      //      }

      //info->reset( );


      if ( ReadResult::NORMAL != rslt ) {
        // something happened so rewind our to our mark
        output( ) << "rewinding to start of segment" << std::endl;
        work.rewindToMark( );

        // ...but maybe we just have an incomplete segment
        // so reading more data will fix our problem
        if ( ReadResult::READER_DEPENDENT != rslt ) {
          // or not, so let the caller know
          return rslt;
        }
      }

      output( ) << "reading more data from input stream" << std::endl;
      output( ) << "before read..." << std::endl;
      output( ) << "work buffer size : " << work.size( ) << std::endl;
      output( ) << "             cap : " << work.capacity( ) << std::endl;
      output( ) << "           avail : " << work.available( ) << std::endl;
      filestream->read( (char*) ( &decodebuffer[0] ), decodebuffer.capacity( ) );
      cnt = filestream->gcount( );
      output( ) << "read " << cnt << " bytes from input" << std::endl;
    }

    output( ) << "file is exhausted" << std::endl;

    // copy any data we have left in our filler set to the real set


    // if we still have stuff in our work buffer, process it
    if ( !work.empty( ) ) {
      output( ) << "still have stuff in our work buffer!" << std::endl;
    }
    copySaved( filler, info );
    return ReadResult::END_OF_FILE;
  }

  void StpReader::copySaved( std::unique_ptr<SignalSet>& from, std::unique_ptr<SignalSet>& to ) {
    // FIXME: use up our leftover waveforms at the current time
    output( ) << "copying temp data to real signalset" << std::endl;
    for ( auto e : from->metadata( ) ) {
      output( ) << e.first << " " << e.second << std::endl;
    }

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

  ReadResult StpReader::processOneChunk( std::unique_ptr<SignalSet>& info ) {
    // make sure we're looking at a header block
    output( ) << "processing one chunk from byte " << work.popped( ) << std::endl;

    output( ) << "work buffer size : " << work.size( ) << std::endl;
    output( ) << "             cap : " << work.capacity( ) << std::endl;
    output( ) << "           avail : " << work.available( ) << std::endl;

    bool blocktypeex = false;
    try {
      // we need to start reading at the beginning of a segment, but sometimes
      // the waveforms appear to end with an extra FA 0D section. My simple
      // (and probably wrong) solution is just to skip ahead until we see the
      // next segment start sequence
      int cnt = 0;
      while ( !( work.read( ) == 0x7E && work.read( 6 ) == 0x7E ) ) {
        work.skip( );
        cnt++;
      }
      if ( cnt > 0 ) {
        output( ) << "  skipped " << cnt << " bytes to find section start at byte: " << work.popped( ) << std::endl;
      }

      // NOTE: we don't know if our working data has a full segment in it,
      // so mark where we started reading, in case we need to rewind
      work.mark( );
      output( ) << "marking at " << work.popped( ) << std::endl;

      work.skip( 18 );
      dr_time timer = popTime( );
      if ( isRollover( currentTime, timer ) ) {
        return ReadResult::END_OF_DAY;
      }
      currentTime = timer;

      work.skip( 2 );
      info->setMeta( "Patient Name", popString( 32 ) );
      work.skip( 2 );
      // offset is number of bytes from byte 64, but we want to track bytes
      // since we started reading (set our mark)
      size_t waveoffset = popUInt16( ) + 64;
      work.skip( 4 ); // don't know what these mean
      work.skip( 2 ); // don't know what these mean, either

      if ( 0x013A == popUInt16( ) ) {
        readDataBlock( info,{ SKIP2, HR, PVC, SKIP4, STI, STII, STIII, STV, SKIP5, STAVR, STAVL, STAVF }, 62 );

        if ( 0x013A != popUInt16( ) ) {
          // we expected a "closing" 0x013A, so something is wrong
          return ReadResult::ERROR;
        }
      }

      while ( work.readSinceMark( ) < waveoffset - 6 ) {
        work.skip( 66 );
        unsigned int blocktype = popUInt8( );
        unsigned int blockfmt = popUInt8( );
        work.rewind( 68 ); // go back to the start of this block
        //output( ) << "new block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
        //    << blocktype << " " << blockfmt << " starting at " << std::dec << work.popped( ) << std::endl;
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
                blocktypeex = true;
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
                blocktypeex = true;
                unhandledBlockType( blocktype, blockfmt );
                break;
            }
            break;
          case 0x04:
            if ( blockfmt == 0x4D ) {
              readDataBlock( info,{ SKIP6, LA1 } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }
            break;
          case 0x05:
            if ( blockfmt == 0x4D ) {
              readDataBlock( info,{ SKIP6, CVP1 } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }
            break;
          case 0x06:
            if ( blockfmt == 0x4D ) {
              readDataBlock( info,{ SKIP6, ICP1, CPP1 } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }
            break;
          case 0x07:
            if ( blockfmt == 0x4D ) {
              readDataBlock( info,{ SKIP6, SP1 } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }
            break;
          case 0x08:
            if ( blockfmt == 0x22 ) {
              readDataBlock( info,{ SKIP6, RESP, APNEA } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }

            break;
          case 0x09:
            if ( blockfmt == 0x22 ) {
              readDataBlock( info,{ SKIP6, BT, IT } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }
            break;
          case 0x0A:
            if ( blockfmt == 0x18 ) {
              readDataBlock( info,{ SKIP6, NBP_M, NBP_S, NBP_D, SKIP2, CUFF } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }
            break;
          case 0x0B:
            if ( blockfmt == 0x2D ) {
              readDataBlock( info,{ SKIP6, SPO2_P, SPO2_R } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }
            break;
          case 0x0C:
            if ( blockfmt == 0x22 ) {
              readDataBlock( info,{ SKIP6, TMP_1, TMP_2, DELTA_TMP } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }
            break;
          case 0x0D:
            readDataBlock( info,{ } );
            //blocktypeex=true; unhandledBlockType( blocktype, blockfmt );
            break;
          case 0x14:
            if ( blockfmt == 0xC2 ) {
              readDataBlock( info,{ SKIP6, PT_RR, PEEP, MV, SKIP2, Fi02, TV, PIP, PPLAT, MAWP, SENS } );
            }
            else {
              blocktypeex = true;
              unhandledBlockType( blocktype, blockfmt );
            }
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
                blocktypeex = true;
                unhandledBlockType( blocktype, blockfmt );
                break;
            }
            break;
          default:
            blocktypeex = true;
            unhandledBlockType( blocktype, blockfmt );
        }
      }
      work.skip( 2 );
      WaveReadResult rslt = readWavesBlock( info );
      if ( WaveReadResult::SKIP_TO_NEXT_SECTION == rslt ) {
        // we have the full section read, but there may be a few extra
        // bytes before the start of the next section
        output( ) << "skip to next section" << std::endl;
      }


      // if no exception has been thrown yet, then we read the entire wave
      // block. It doesn't matter if we stopped at the end of a segment
      // or at the end of the work buffer
      return ReadResult::NORMAL;
    }
    catch ( const std::runtime_error & err ) {
      if ( blocktypeex ) {
        output( ) << "exception occurred: " << err.what( ) << " at byte: " << std::dec << work.popped( ) << std::endl;
        return ReadResult::ERROR;
      }
      else {
        // hopefully, we just need a little more data to read a full segment
        output( ) << "ran out of data in work buffer" << std::endl;
        return ReadResult::READER_DEPENDENT;
      }
    }
  }

  void StpReader::unhandledBlockType( unsigned int type, unsigned int fmt ) const {
    std::stringstream ss;
    ss << "unhandled block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
        << type << " " << fmt << " starting at " << std::dec << work.readSinceMark( );
    std::string ex = ss.str( );
    throw std::runtime_error( ss.str( ) );
  }

  unsigned int StpReader::popUInt16( ) {
    unsigned char b1 = work.pop( );
    unsigned char b2 = work.pop( );
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
    for ( size_t i = 0; i < data.size( ); i++ ) {
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

  StpReader::WaveReadResult StpReader::readWavesBlock( std::unique_ptr<SignalSet>& info ) {
    output( ) << "waves section starts at: " << std::dec << work.popped( ) << std::endl;
    work.skip( 4 );
    output( ) << "first wave at: " << work.popped( ) << std::endl;

    static const unsigned int READCOUNTS[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    std::map<int, unsigned int> expectedValues;
    std::map<int, std::vector<int>> wavevals;

    if ( work.popped( ) >= 15449069 ) {
      output( ) << "here 0!";
    }

    int fa0dloops = 0;
    // basically, we're just going to keep reading until we hit the next
    // segment...then write the appropriate number of values to the signalset
    // and keep any overrun for the next loop
    while ( 0x7E != work.read( ) ) {
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
        output( ) << "FA 0D section at byte: " << std::dec << ( work.popped( ) - 2 ) << " (loop " << fa0dloops << ")" << std::endl;
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
      else if ( !( countbyte == 0x0B || countbyte == 0x3B || countbyte == 0xFB || countbyte == 0x09 ) ) {
        std::stringstream ss;
        ss << "unhandled wave count for known id: "
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
        output( ) << "reading " << valstoread
            << " values starting at " << std::dec << work.readSinceMark( ) << std::endl;
        if ( countbyte > 0x0C ) {
          // we had a 0x3B or something
          output( ) << "countbyte discrepancy! "
              << std::setfill( '0' ) << std::setw( 2 ) << std::hex << countbyte
              << " at byte " << std::dec << ( work.popped( ) - 1 ) << std::endl;
        }

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
    for ( auto& en : wavevals ) {
      leftoverwaves[en.first].insert( leftoverwaves[en.first].end( ), en.second.begin( ), en.second.end( ) );
    }

    for ( auto& w : wavevals ) {
      if ( leftoverwaves[w.first].size( ) != expectedValues[w.first] ) {
        output( ) << "wave " << w.first << " read " << w.second.size( )
            << " values for a total of " << leftoverwaves[w.first].size( )
            << " expecting to write " << expectedValues[w.first] << " values..." << std::endl;
      }
      std::stringstream vals;
      std::vector<int>& vector = leftoverwaves.at( w.first );

      if ( vector.size( ) != expectedValues[w.first] ) {
        int missingcount = expectedValues[w.first] - vector.size( );
        output( ) << "filling in " << missingcount << " missing values for wave wave " << w.first << std::endl;

        for ( int i = 0; i < missingcount; i++ ) {
          vector.push_back( SignalData::MISSING_VALUE );
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
          signal->setChunkIntervalAndSampleRate( 2000, w.second.size( ) );
        }

        signal->add( DataRow( currentTime, vals.str( ) ) );
      }
      //      else {
      //        output( ) << "(wave not ok)";
      //      }
      vector.erase( vector.begin( ), vector.begin( ) + expectedValues[w.first] );
      //      output( ) << "after writing, " << vector.size( ) << " vals left for next loop" << std::endl;
    }

    return WaveReadResult::SKIP_TO_NEXT_SECTION;
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
}