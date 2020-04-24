/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ZlReader2.h
 * Author: ryan
 *
 */

#ifndef STPREADER_H
#define STPREADER_H

#include "Reader.h"
#include "StreamChunkReader.h"
#include <map>
#include <string>
#include <memory>
#include <istream>
#include "zstr.hpp"
#include "CircularBuffer.h"

namespace FormatConverter {
  class SignalData;
  class StreamChunkReader;

  /**
   * A reader for the HSDI signal files that UVA is producing
   */
  class StpReader : public Reader {
  public:
    StpReader( );
    StpReader( const std::string& name );
    virtual ~StpReader( );

    struct StpMetadata {
      std::string name;
      std::string mrn;
      dr_time start_utc;
      dr_time stop_utc;
      int segment_count;
    };

    static std::vector<StpMetadata> parseMetadata( const std::string& input );

  protected:
    ReadResult fill( std::unique_ptr<SignalSet>&, const ReadResult& lastfill ) override;
    int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
    void finish( ) override;

  private:
    static StpMetadata metaFromSignalSet( const std::unique_ptr<SignalSet>& );

    enum ChunkReadResult {
      OK, ROLLOVER, NEW_PATIENT, UNKNOWN_BLOCKTYPE, HR_BLOCK_PROBLEM
    };

    enum WaveSequenceResult {
      NORMAL, DUPLICATE, SEQBREAK, TIMEBREAK
    };

    class WaveTracker {
    public:
      WaveTracker( );
      virtual ~WaveTracker( );

      WaveSequenceResult newseq( const unsigned short& seqnum, dr_time time );
      void newvalues( int waveid, std::vector<int>& values );
      void reset( );
      /**
       * Have we seen at least 8 FA0D loops?
       * @return
       */
      bool writable( ) const;
      /**
       * Write any data we have to the signal set, filling in missing values
       * as necessary. Also, update the sequence counting and timer values
       * @param signals
       */
      void flushone( std::unique_ptr<SignalSet>& signals );
      bool empty( ) const;

      unsigned short currentseq( ) const;
      const dr_time starttime( ) const;
      dr_time vitalstarttime( ) const;

    private:
      void prune( );
      void breaksync( WaveSequenceResult rslt,
          const unsigned short& seqnum, const dr_time& time );

      std::vector<std::pair<unsigned short, dr_time>> sequencenums;
      std::map<int, std::vector<int>> wavevals;
      std::map<int, size_t> expectedValues;
      std::map<int, size_t> miniseen;
      dr_time mytime;
    };

    class BlockConfig {
    public:
      const bool isskip;
      const std::string label;
      const unsigned int divBy10;
      const size_t readcount;
      const bool unsign;
      const std::string uom;

      static BlockConfig skip( size_t count = 1 ) {
        return BlockConfig( count );
      }

      static BlockConfig vital( const std::string& lbl, const std::string& uom,
          size_t read = 2, bool unsign = true ) {
        return BlockConfig( lbl, read, 0, unsign, uom );
      }

      static BlockConfig div10( const std::string& lbl, const std::string& uom,
          size_t read = 2, bool unsign = true ) {
        return BlockConfig( lbl, read, 1, unsign, uom );
      }

      static BlockConfig div100( const std::string& lbl, const std::string& uom,
          size_t read = 2, bool unsign = true ) {
        return BlockConfig( lbl, read, 2, unsign, uom );
      }

    private:

      BlockConfig( int read = 1 )
          : isskip( true ), label( "SKIP" ), divBy10( false ), readcount( read ), unsign( false ),
          uom( "Uncalib" ) { }

      BlockConfig( const std::string& lbl, size_t read = 2, unsigned int div = 0, bool unsign = true, const std::string& uom = "" )
          : isskip( false ), label( lbl ), divBy10( div ), readcount( read ), unsign( unsign ),
          uom( uom ) { }
    };

    // <editor-fold defaultstate="collapsed" desc="block configs">
    static const BlockConfig SKIP;
    static const BlockConfig SKIP2;
    static const BlockConfig SKIP4;
    static const BlockConfig SKIP5;
    static const BlockConfig SKIP6;
    static const BlockConfig HR;
    static const BlockConfig PVC;
    static const BlockConfig STI;
    static const BlockConfig STII;
    static const BlockConfig STIII;
    static const BlockConfig STAVR;
    static const BlockConfig STAVL;
    static const BlockConfig STAVF;
    static const BlockConfig STV;
    static const BlockConfig STV1;
    static const BlockConfig BT;
    static const BlockConfig IT;
    static const BlockConfig RESP;
    static const BlockConfig APNEA;
    static const BlockConfig NBP_M;
    static const BlockConfig NBP_S;
    static const BlockConfig NBP_D;
    static const BlockConfig CUFF;
    static const BlockConfig AR1_M;
    static const BlockConfig AR1_S;
    static const BlockConfig AR1_D;
    static const BlockConfig AR1_R;
    static const BlockConfig AR2_M;
    static const BlockConfig AR2_S;
    static const BlockConfig AR2_D;
    static const BlockConfig AR2_R;
    static const BlockConfig AR3_M;
    static const BlockConfig AR3_S;
    static const BlockConfig AR3_D;
    static const BlockConfig AR3_R;
    static const BlockConfig AR4_M;
    static const BlockConfig AR4_S;
    static const BlockConfig AR4_D;
    static const BlockConfig AR4_R;

    static const BlockConfig SPO2_P;
    static const BlockConfig SPO2_R;
    static const BlockConfig VENT;
    static const BlockConfig IN_HLD;
    static const BlockConfig PRS_SUP;
    static const BlockConfig INSP_TM;
    static const BlockConfig INSP_PC;
    static const BlockConfig I_E;
    static const BlockConfig SET_PCP;
    static const BlockConfig SET_IE;
    static const BlockConfig APRV_LO_T;
    static const BlockConfig APRV_HI_T;
    static const BlockConfig APRV_LO;
    static const BlockConfig APRV_HI;
    static const BlockConfig RESIS;
    static const BlockConfig MEAS_PEEP;
    static const BlockConfig INTR_PEEP;
    static const BlockConfig INSP_TV;
    static const BlockConfig COMP;

    static const BlockConfig HF_FLW;
    static const BlockConfig B_FLW;
    static const BlockConfig FLW_TRIG;
    static const BlockConfig FLW_R;
    static const BlockConfig HF_R;
    static const BlockConfig HF_PRS;
    static const BlockConfig TMP_1;
    static const BlockConfig TMP_2;
    static const BlockConfig DELTA_TMP;
    static const BlockConfig LA1;
    static const BlockConfig CVP1;
    static const BlockConfig CVP2;
    static const BlockConfig CVP3;
    static const BlockConfig CVP4;
    static const BlockConfig ICP1;
    static const BlockConfig CPP1;
    static const BlockConfig ICP2;
    static const BlockConfig CPP2;
    static const BlockConfig ICP3;
    static const BlockConfig CPP3;
    static const BlockConfig ICP4;
    static const BlockConfig CPP4;
    static const BlockConfig SP1;
    static const BlockConfig PA1_M;
    static const BlockConfig PA1_S;
    static const BlockConfig PA1_D;
    static const BlockConfig PA1_R;
    static const BlockConfig PA2_M;
    static const BlockConfig PA2_S;
    static const BlockConfig PA2_D;
    static const BlockConfig PA2_R;
    static const BlockConfig PA3_M;
    static const BlockConfig PA3_S;
    static const BlockConfig PA3_D;
    static const BlockConfig PA3_R;
    static const BlockConfig PA4_M;
    static const BlockConfig PA4_S;
    static const BlockConfig PA4_D;
    static const BlockConfig PA4_R;

    static const BlockConfig UAC1_M;
    static const BlockConfig UAC1_S;
    static const BlockConfig UAC1_D;
    static const BlockConfig UAC1_R;
    static const BlockConfig UAC2_M;
    static const BlockConfig UAC2_S;
    static const BlockConfig UAC2_D;
    static const BlockConfig UAC2_R;
    static const BlockConfig UAC3_M;
    static const BlockConfig UAC3_S;
    static const BlockConfig UAC3_D;
    static const BlockConfig UAC3_R;
    static const BlockConfig UAC4_M;
    static const BlockConfig UAC4_S;
    static const BlockConfig UAC4_D;
    static const BlockConfig UAC4_R;

    static const BlockConfig PT_RR;
    static const BlockConfig PEEP;
    static const BlockConfig MV;
    static const BlockConfig Fi02;
    static const BlockConfig TV;
    static const BlockConfig PIP;
    static const BlockConfig PPLAT;
    static const BlockConfig MAWP;
    static const BlockConfig SENS;
    static const BlockConfig SPONT_MV;
    static const BlockConfig SPONT_R;
    static const BlockConfig SET_TV;

    static const BlockConfig CO2_EX;
    static const BlockConfig CO2_IN;
    static const BlockConfig CO2_RR;
    static const BlockConfig O2_EXP;
    static const BlockConfig O2_INSP; // </editor-fold>

    static const std::map<int, std::string> WAVELABELS;

    StpReader( const StpReader& orig );
    ChunkReadResult processOneChunk( std::unique_ptr<SignalSet>&, const size_t& maxread );
    dr_time popTime( );
    std::string popString( size_t length );
    int popInt16( );
    int popInt8( );
    unsigned int popUInt8( );
    unsigned int popUInt16( );
    unsigned int readUInt16( );
    unsigned long popUInt64( );

    static std::string wavelabel( int waveid, const std::unique_ptr<SignalSet>& );

    void readDataBlock( std::unique_ptr<SignalSet>& info, const std::vector<BlockConfig>& vitals, size_t blocksize = 68 );
    static std::string div10s( int val, unsigned int multiple = 1 );

    void unhandledBlockType( unsigned int type, unsigned int fmt ) const;
    ChunkReadResult readWavesBlock( std::unique_ptr<SignalSet>& info, const size_t& maxread );

    /**
     * determines if the work buffer has at least one full segment in it
     * @param size the size of the first segment in the work buffer, if a complete segment exists
     * @return
     */
    bool workHasFullSegment( size_t * size = nullptr );

    bool isunity( ) const;

    bool firstread;
    dr_time currentTime;
    zstr::ifstream * filestream;
    CircularBuffer<unsigned char> work;
    std::vector<unsigned char> decodebuffer;
    unsigned long magiclong;
    WaveTracker wavetracker;
    bool metadataonly;
  };
}
#endif /* STPREADER_H */

