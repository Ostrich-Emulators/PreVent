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

#ifndef STPPHOILIPSREADER_H
#define STPPHOILIPSREADER_H

#include "StpReaderBase.h"

namespace FormatConverter {
  class SignalData;
  class StreamChunkReader;

  /**
   * A reader for the HSDI signal files that UVA is producing
   */
  class StpPhilipsReader : public StpReaderBase {
  public:
    StpPhilipsReader( const std::string& name = "STP (Philips)" );
    virtual ~StpPhilipsReader( );

    struct StpMetadata {
      std::string name;
      std::string mrn;
      dr_time start_utc;
      dr_time stop_utc;
      int segment_count;
    };

    static std::vector<StpMetadata> parseMetadata( const std::string& input );

  protected:
    virtual ReadResult fill( std::unique_ptr<SignalSet>&, const ReadResult& lastfill ) override;
    virtual int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;

  private:
    static StpMetadata metaFromSignalSet( const std::unique_ptr<SignalSet>& );

    enum ChunkReadResult {
      OK, ROLLOVER, NEW_PATIENT, UNKNOWN_BLOCKTYPE, HR_BLOCK_PROBLEM
    };

    static const std::map<int, std::string> WAVELABELS;

    StpPhilipsReader( const StpPhilipsReader& orig );
    ChunkReadResult processOneChunk( std::unique_ptr<SignalSet>&, const size_t& maxread );
    dr_time popTime( );

    /**
     * determines if the work buffer has at least one full segment in it
     * @param size the size of the first segment in the work buffer, if a complete segment exists
     * @return
     */
    bool workHasFullSegment( size_t * size = nullptr );

    bool firstread;
    dr_time currentTime;
  };
}
#endif /* STPPHOILIPSREADER_H */

