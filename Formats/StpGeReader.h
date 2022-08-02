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

#ifndef STPGEREADER_H
#define STPGEREADER_H

#include "StpReaderBase.h"
#include "StpGeSegment.h"

#include <map>
#include <string>
#include <memory>
#include <istream>
#include "zstr.hpp"

namespace FormatConverter {
  class SignalData;
  class StreamChunkReader;

  /**
   * A reader for the HSDI signal files that UVA is producing
   */
  class StpGeReader : public StpReaderBase {
  public:
    StpGeReader( const std::string& name = "STP (GE)" );
    virtual ~StpGeReader( );

    static std::vector<StpMetadata> parseMetadata( const std::string& input );

  protected:
    virtual ReadResult fill( SignalSet *, const ReadResult& lastfill ) override;
    virtual int prepare( const std::string& input, SignalSet * info ) override;

  private:
    static StpMetadata metaFromSignalSet( const std::unique_ptr<SignalSet>& );

    enum ChunkReadResult {
      OK, ROLLOVER, NEW_PATIENT, DECODE_ERROR
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
      void flushone( SignalSet * signals );
      bool empty( ) const;

      unsigned short currentseq( ) const;
      const dr_time starttime( ) const;
      dr_time vitalstarttime( ) const;
    private:

      class SequenceData {
      public:
        SequenceData( unsigned short, dr_time );
        SequenceData( const SequenceData&);
        SequenceData& operator=( const SequenceData& );

        ~SequenceData( );
        unsigned short sequence;
        dr_time time;

        void push( int waveid, const std::vector<int>& data );
        std::map<int, std::vector<int>> wavedata; // wave id, values
      private:
      };

      void prune( );
      void breaksync( WaveSequenceResult rslt,
          const unsigned short& currseq, const unsigned short& newseq,
          const dr_time& currtime, const dr_time& newtime );

      std::vector<SequenceData> sequencedata;
      std::map<int, size_t> expectedValues;
      std::map<int, size_t> miniseen;
    };

    static const std::map<int, std::string> WAVELABELS;

    StpGeReader( const StpGeReader& orig );
    ChunkReadResult processOneChunk( SignalSet *, const std::vector<unsigned char>& chunkbytes );

    static std::string wavelabel( int waveid, SignalSet * );

    void readDataBlock( const std::vector<unsigned char>& workdata,
        const StpGeSegment::VitalsBlock& vitals, SignalSet * info );
    static std::string div10s( int val, unsigned int multiple = 1 );

    ChunkReadResult readWavesBlock( const std::vector<unsigned char>& workdata,
        const std::unique_ptr<StpGeSegment>& index, SignalSet * info );

    bool popToNextSegment();

    /**
     * determines if the work buffer has at least one full segment in it
     * @param size the size of the first segment in the work buffer, if a complete segment exists
     * @return
     */
    bool workHasFullSegment( size_t * size = nullptr );

    bool isunity( ) const;

    bool firstread;
    dr_time currentTime;
    unsigned long magiclong;
    WaveTracker wavetracker;
    int catchup;
    bool catchupEven; // we only use every other catchup value
  };
}
#endif /* STPGEREADER_H */

