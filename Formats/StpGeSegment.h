/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StepGeFile.h
 * Author: ryan
 *
 */

#ifndef STPGESEGMENT_H
#define STPGESEGMENT_H

#include "CircularBuffer.h"

#include <map>
#include <string>
#include <memory>
#include <istream>

#include "dr_time.h"

namespace FormatConverter {
  class SignalData;
  class StreamChunkReader;

  /**
   * A reader for the HSDI signal files that UVA is producing
   */
  class StpGeSegment {
  public:

    enum GEParseError {
      NO_ERROR,
      UNKNOWN_VITALSTYPE,
      VITALSBLOCK_OVERFLOW,
      WAVE_INCONSISTENT_VALCOUNT,
      WAVE_TOO_MANY_VALUES
    };

    virtual ~StpGeSegment( );

    class Header {
    public:
      /**
       * Reads the header segment that starts at the start of the raw data
       */
      static Header parse( const std::vector<unsigned char>& rawdata );
      Header( const Header& );
      const dr_time time;
      const std::string patient;
      const bool unity;

    private:
      Header( unsigned long magic = 0, dr_time t = 0, const std::string& patient = "" );
      unsigned long magic;
    };

    class VitalsBlock {
    public:

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

        BlockConfig( int read = 1 ) : isskip( true ), label( "SKIP" + std::to_string( read ) ),
            divBy10( false ), readcount( read ), unsign( false ), uom( "Uncalib" ) { }

        BlockConfig( const std::string& lbl, size_t read = 2, unsigned int div = 0,
            bool unsign = true, const std::string& uom = "" )
            : isskip( false ), label( lbl ), divBy10( div ), readcount( read ),
            unsign( unsign ), uom( uom ) { }
      };

      // <editor-fold defaultstate="collapsed" desc="block configs">
      static const BlockConfig BC_SKIP;
      static const BlockConfig BC_SKIP2;
      static const BlockConfig BC_SKIP4;
      static const BlockConfig BC_SKIP5;
      static const BlockConfig BC_SKIP6;
      static const BlockConfig BC_HR;
      static const BlockConfig BC_PVC;
      static const BlockConfig BC_STI;
      static const BlockConfig BC_STII;
      static const BlockConfig BC_STIII;
      static const BlockConfig BC_STAVR;
      static const BlockConfig BC_STAVL;
      static const BlockConfig BC_STAVF;
      static const BlockConfig BC_STV;
      static const BlockConfig BC_STV1;
      static const BlockConfig BC_STV2;
      static const BlockConfig BC_STV3;
      static const BlockConfig BC_BT;
      static const BlockConfig BC_IT;
      static const BlockConfig BC_RESP;
      static const BlockConfig BC_APNEA;
      static const BlockConfig BC_NBP_M;
      static const BlockConfig BC_NBP_S;
      static const BlockConfig BC_NBP_D;
      static const BlockConfig BC_CUFF;

      static const BlockConfig BC_AR1_M;
      static const BlockConfig BC_AR1_S;
      static const BlockConfig BC_AR1_D;
      static const BlockConfig BC_AR1_R;
      static const BlockConfig BC_AR2_M;
      static const BlockConfig BC_AR2_S;
      static const BlockConfig BC_AR2_D;
      static const BlockConfig BC_AR2_R;
      static const BlockConfig BC_AR3_M;
      static const BlockConfig BC_AR3_S;
      static const BlockConfig BC_AR3_D;
      static const BlockConfig BC_AR3_R;
      static const BlockConfig BC_AR4_M;
      static const BlockConfig BC_AR4_S;
      static const BlockConfig BC_AR4_D;
      static const BlockConfig BC_AR4_R;

      static const BlockConfig BC_SPO2_P;
      static const BlockConfig BC_SPO2_R;
      static const BlockConfig BC_VENT;
      static const BlockConfig BC_IN_HLD;
      static const BlockConfig BC_PRS_SUP;
      static const BlockConfig BC_INSP_TM;
      static const BlockConfig BC_INSP_PC;
      static const BlockConfig BC_I_E;
      static const BlockConfig BC_SET_PCP;
      static const BlockConfig BC_SET_IE;
      static const BlockConfig BC_APRV_LO_T;
      static const BlockConfig BC_APRV_HI_T;
      static const BlockConfig BC_APRV_LO;
      static const BlockConfig BC_APRV_HI;
      static const BlockConfig BC_RESIS;
      static const BlockConfig BC_MEAS_PEEP;
      static const BlockConfig BC_INTR_PEEP;
      static const BlockConfig BC_INSP_TV;
      static const BlockConfig BC_COMP;

      static const BlockConfig BC_HF_FLW;
      static const BlockConfig BC_B_FLW;
      static const BlockConfig BC_FLW_TRIG;
      static const BlockConfig BC_FLW_R;
      static const BlockConfig BC_HF_R;
      static const BlockConfig BC_HF_PRS;
      static const BlockConfig BC_TMP_1;
      static const BlockConfig BC_TMP_2;
      static const BlockConfig BC_DELTA_TMP;

      static const BlockConfig BC_LA1;
      static const BlockConfig BC_LA2;
      static const BlockConfig BC_LA3;
      static const BlockConfig BC_LA4;

      static const BlockConfig BC_CVP1;
      static const BlockConfig BC_CVP2;
      static const BlockConfig BC_CVP3;
      static const BlockConfig BC_CVP4;

      static const BlockConfig BC_ICP1;
      static const BlockConfig BC_ICP2;
      static const BlockConfig BC_ICP3;
      static const BlockConfig BC_ICP4;

      static const BlockConfig BC_CPP1;
      static const BlockConfig BC_CPP2;
      static const BlockConfig BC_CPP3;
      static const BlockConfig BC_CPP4;

      static const BlockConfig BC_SP1;

      static const BlockConfig BC_PA1_M;
      static const BlockConfig BC_PA1_S;
      static const BlockConfig BC_PA1_D;
      static const BlockConfig BC_PA1_R;
      static const BlockConfig BC_PA2_M;
      static const BlockConfig BC_PA2_S;
      static const BlockConfig BC_PA2_D;
      static const BlockConfig BC_PA2_R;
      static const BlockConfig BC_PA3_M;
      static const BlockConfig BC_PA3_S;
      static const BlockConfig BC_PA3_D;
      static const BlockConfig BC_PA3_R;
      static const BlockConfig BC_PA4_M;
      static const BlockConfig BC_PA4_S;
      static const BlockConfig BC_PA4_D;
      static const BlockConfig BC_PA4_R;

      static const BlockConfig BC_FE1_M;
      static const BlockConfig BC_FE1_S;
      static const BlockConfig BC_FE1_D;
      static const BlockConfig BC_FE1_R;
      static const BlockConfig BC_FE2_M;
      static const BlockConfig BC_FE2_S;
      static const BlockConfig BC_FE2_D;
      static const BlockConfig BC_FE2_R;
      static const BlockConfig BC_FE3_M;
      static const BlockConfig BC_FE3_S;
      static const BlockConfig BC_FE3_D;
      static const BlockConfig BC_FE3_R;
      static const BlockConfig BC_FE4_M;
      static const BlockConfig BC_FE4_S;
      static const BlockConfig BC_FE4_D;
      static const BlockConfig BC_FE4_R;

      static const BlockConfig BC_UAC1_M;
      static const BlockConfig BC_UAC1_S;
      static const BlockConfig BC_UAC1_D;
      static const BlockConfig BC_UAC1_R;
      static const BlockConfig BC_UAC2_M;
      static const BlockConfig BC_UAC2_S;
      static const BlockConfig BC_UAC2_D;
      static const BlockConfig BC_UAC2_R;
      static const BlockConfig BC_UAC3_M;
      static const BlockConfig BC_UAC3_S;
      static const BlockConfig BC_UAC3_D;
      static const BlockConfig BC_UAC3_R;
      static const BlockConfig BC_UAC4_M;
      static const BlockConfig BC_UAC4_S;
      static const BlockConfig BC_UAC4_D;
      static const BlockConfig BC_UAC4_R;

      static const BlockConfig BC_PT_RR;
      static const BlockConfig BC_PEEP;
      static const BlockConfig BC_MV;
      static const BlockConfig BC_Fi02;
      static const BlockConfig BC_TV;
      static const BlockConfig BC_PIP;
      static const BlockConfig BC_PPLAT;
      static const BlockConfig BC_MAWP;
      static const BlockConfig BC_SENS;
      static const BlockConfig BC_SPONT_MV;
      static const BlockConfig BC_SPONT_R;
      static const BlockConfig BC_SET_TV;

      static const BlockConfig BC_CO2_EX;
      static const BlockConfig BC_CO2_IN;
      static const BlockConfig BC_CO2_RR;
      static const BlockConfig BC_O2_EXP;
      static const BlockConfig BC_O2_INSP;
      static const BlockConfig BC_RWOBVT;
      static const BlockConfig BC_RI_E;
      // </editor-fold>

      enum MajorMode {
        STANDARD = 0x01
      };

      enum Signal {
        NBP = 0x18,
        RESP = 0x22,
        TEMP = 0x23,
        SPO2 = 0x2D,
        CO2 = 0x36,
        HR = 0x3A,
        MSDR1 = 0x4D,
        MSDR2 = 0x4E,
        MSDR3 = 0x4F,
        MSDR4 = 0x50,
        ST1 =  0x56,
        STV = 0x57,
        NONE1 = 0x58,
        STAVL = 0x59,
        RWOBVT = 0x5A,
        RIE = 0x5B,
        APRV = 0x5C,
        // TV = 0x5D,
        AR1 = 0x82,
        AR2 = 0x83,
        AR3 = 0x84,
        AR4 = 0x85,
        PA1 = 0xBC,
        PA2 = 0xBD,
        PA3 = 0xBE,
        PA4 = 0xBF,
        PEEP = 0xC2,
        FLOW = 0xDB,
        HF = 0xDC
      };

      /**
       * Reads one vitals block that starts at pos
       *
       * @param rawdata the data
       * @param pos where in the data to start reading
       * @param errcode if any error was raised during parsing (0==no error)
       * @return the vitals information
       */
      static VitalsBlock index( const std::vector<unsigned char>& rawdata, unsigned long pos,
          GEParseError& errcode );
      const MajorMode major;
      const Signal signal;
      const int lead;
      const unsigned int datastart;
      const std::vector<BlockConfig>& config;
      int hex() const;

    private:
      static const std::map<Signal, std::map<int, std::vector<BlockConfig>>> LOOKUP;

      VitalsBlock( MajorMode major, Signal code, int lead, unsigned int start,
        const std::vector<BlockConfig>& cfg );
    };

    class WaveFA0DBlock {
    public:
      class WaveFormIndex {
      public:
        WaveFormIndex( unsigned int w, unsigned int cnt, unsigned int datastart);
        const unsigned int waveid;
        const unsigned int valcount;
        const unsigned int datastart;
      };

      /**
       * Reads one vitals block that starts at the start
       * @param start where to start reading wave data. After this function, it points to the
       * point past where the wave block ends (start of the next wave block?)
       */
      static WaveFA0DBlock index( const std::vector<unsigned char>& rawdata, unsigned long &start,
          const std::vector<VitalsBlock>& vitals, GEParseError& errcode );
      const std::vector<WaveFormIndex> wavedata;
      const int sequence;
      const unsigned long startpos;
      const unsigned long endpos;

    private:
      static const unsigned int READCOUNTS[];

      WaveFA0DBlock( int sequence, unsigned long start, unsigned long end,
          const std::vector<WaveFormIndex>& data );
    };

    static std::unique_ptr<StpGeSegment> index( const std::vector<unsigned char>& data, bool skipwaves,
        GEParseError& err );

    const Header header;
    const std::vector<VitalsBlock> vitals;
    const std::vector<WaveFA0DBlock> waves;

    static unsigned int readUInt( const std::vector<unsigned char>& rawdata, unsigned long pos );
    static unsigned int readUInt2( const std::vector<unsigned char>& rawdata, unsigned long pos );
    static unsigned long readUInt4( const std::vector<unsigned char>& rawdata, unsigned long pos );

    static int readInt( const std::vector<unsigned char>& rawdata, unsigned long pos );
    static int readInt2( const std::vector<unsigned char>& rawdata, unsigned long pos );
    static long readInt4( const std::vector<unsigned char>& rawdata, unsigned long pos );

  private:
    StpGeSegment( Header h, const std::vector<VitalsBlock>& vitals = std::vector<VitalsBlock>( ),
        const std::vector<WaveFA0DBlock>& waves = std::vector<WaveFA0DBlock>( ) );
    static const int HEADER_SIZE;
    static const int VITALS_SIZE;
  };
}
#endif /* STPGESEGMENT_H */

