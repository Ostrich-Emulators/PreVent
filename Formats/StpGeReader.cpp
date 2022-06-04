/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "StpGeReader.h"
#include "SignalData.h"
#include "DataRow.h"
#include "BasicSignalSet.h"
#include "Log.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <filesystem>
#include "config.h"
#include "StpGeSegment.h"

namespace FormatConverter {

  /**
   * WARNING: wave labels can change depending on what vitals are in the file, so
   * it's best not to use this lookup directly. Use wavelabel() instead
   */
  const std::map<int, std::string> StpGeReader::WAVELABELS = {
    {0x07, "I" },
    {0x08, "II" },
    {0x09, "III" },
    {0x0A, "V" },
    {0x0B, "AVR" },
    {0x0C, "AVF" },
    {0x0D, "AVL" },
    {0x17, "RR" },
    {0x1B, "AR1" }, // or ICP, or PA, or FE
    {0x1C, "AR2" }, // or ICP, or PA, or FE
    {0x1D, "AR3" }, // or ICP, or PA, or FE
    {0x1E, "AR4" }, // or ICP, or PA, or FE
    {0x27, "SPO2" },
    {0x2A, "CO2" },
    {0xC8, "VNT_PRES" },
    {0xC9, "VNT_FLOW" },
  };

  // <editor-fold defaultstate="collapsed" desc="Wave Tracker">

  StpGeReader::WaveTracker::WaveTracker( ) {
  }

  StpGeReader::WaveTracker::~WaveTracker( ) {
  }

  void StpGeReader::WaveTracker::prune( ) {

    std::vector<int> toremove;
    for ( auto& m : wavevals ) {
      if ( m.second.empty( ) ) {
        toremove.push_back( m.first );
      }
    }

    for ( int waveid : toremove ) {
      wavevals.erase( waveid );
      expectedValues.erase( waveid );
    }

    if ( !wavevals.empty( ) ) {
      Log::error( ) << "NO sequence numbers, but wave vals? WRONG!" << std::endl;
      for ( const auto& m : wavevals ) {
        Log::error( ) << "\twave: " << m.first << "\t" << m.second.size( ) << std::endl;
      }
    }
  }

  StpGeReader::WaveSequenceResult StpGeReader::WaveTracker::newseq( const unsigned short& seqnum, dr_time time ) {
    WaveSequenceResult rslt;
    if ( empty( ) ) {
      rslt = WaveSequenceResult::NORMAL;
      prune( );
      mytime = time;
    }
    else {
      unsigned short currseq = currentseq( );
      size_t timediff = time - starttime( );

      // we get 4 sequence numbers per second, so they duplicate every 64 seconds
      // thus, if our times are out of whack by >=64 seconds, we're sure our
      // times have diverged
      if ( timediff > 64000 ) {
        rslt = WaveSequenceResult::TIMEBREAK;
      }
      else {
        if ( 0 == currseq ) {
          rslt = WaveSequenceResult::NORMAL;
        }
        else if ( seqnum == ( currseq + 1 ) ) {
          // sometimes, we get the right sequence number, but the time is off
          if ( time - mytime >= 4000 ) { // 4s is arbitrary
            // sequence is right, but the time is off, so figure out what
            // the time should be by reading what's in our chute
            size_t counter = 0;
            auto it = sequencenums.rbegin( );
            dr_time xtime = it->second;
            while ( it++ != sequencenums.rend( ) && ++counter < 8 && it->second == xtime ) {
              // everything is handled in the loop check
            }
            time = xtime;
            if ( 8 == counter ) {
              time += 2000;
            }
          }

          rslt = WaveSequenceResult::NORMAL;
        }
        else if ( seqnum == currseq ) {
          rslt = WaveSequenceResult::DUPLICATE;
        }
        else {
          // we will roll-over short limits eventually (and that's ok)
          rslt = ( 0xFF == currseq && 0x00 == seqnum
              ? WaveSequenceResult::NORMAL
              : WaveSequenceResult::SEQBREAK );
        }
      }

      // if we have a break, fill up the current 2-second block we're on
      // then fill in the number of seconds until we're back in sync
      if ( WaveSequenceResult::SEQBREAK == rslt || WaveSequenceResult::TIMEBREAK == rslt ) {
        breaksync( rslt, seqnum, time );
      }
    }

    sequencenums.push_back( std::make_pair( seqnum, time ) );
    miniseen.clear( );
    return rslt;
  }

  void StpGeReader::WaveTracker::breaksync( StpGeReader::WaveSequenceResult rslt,
      const unsigned short& seqnum, const dr_time& time ) {

    // for timed breaks, we really just want to fill up the current second
    // with missing data.

    // for sequence breaks, we don't want to fill up the whole second...just
    // the missing sequences.

    // if we're missing >8 elements, treat it as if it's a timed break

    unsigned short currseq = currentseq( );

    // the -1 is because we haven't read the seqnum values yet
    // we don't use these variables if it's a timedbreak
    unsigned short seqcalc = ( seqnum - 1 < currseq ? seqnum + 0xFF : seqnum - 1 );
    unsigned short seqdiff = ( seqcalc - currseq );

    // arbitrary: if the sequence break is more than 8 elements, just fill
    // in the current second as if it was a timed break
    if ( WaveSequenceResult::TIMEBREAK == rslt || seqdiff > 8 ) {
      // just add phantom sequence numbers to fill up the current block and we're done
      // first, we need to figure out how many sequence numbers to add by counting
      // how many we have at the current time
      const dr_time ctime = ( --sequencenums.end( ) )->second;
      size_t count = std::count_if( sequencenums.begin( ), sequencenums.end( ),
          [ctime]( std::pair<unsigned short, dr_time> p ) {
            return (p.second == ctime );
          } );
      for ( size_t lim = 1; lim <= ( 8 - count ); lim++ ) {
        sequencenums.push_back( std::make_pair( ( currseq + lim ) % 0xFF, ctime ) );
      }

      // fill in MISSING values into our datapoints, too
      for ( auto& w : wavevals ) {
        const int waveid = w.first;
        std::vector<int>& datapoints = w.second;
        size_t loops = 8 - count;
        Log::trace( ) << "wave " << waveid << ": before filling: " << datapoints.size( );
        // break is based on missing sequence numbers
        size_t valsPerSeqNum = expectedValues[waveid] / 8; // 8 FA0D loops/sec
        size_t valsToAdd = loops * valsPerSeqNum;
        datapoints.resize( datapoints.size( ) + valsToAdd, SignalData::MISSING_VALUE );

        Log::trace( ) << "; after filling: " << datapoints.size( ) << std::endl;
      }
    }
    else {

      Log::trace( ) << "small break in sequence (" << seqdiff << " elements) current map:" << std::endl;
      for ( const auto& m : sequencenums ) {
        Log::trace( ) << "\t" << m.first << "\t" << m.second << std::endl;
      }

      // we have a small sequence difference, so fill in phantom values
      for ( auto& w : wavevals ) {
        const int waveid = w.first;
        std::vector<int>& datapoints = w.second;

        Log::trace( ) << "wave " << waveid << ": before filling: " << datapoints.size( );
        // break is based on missing sequence numbers
        size_t valsPerSeqNum = expectedValues[waveid] / 8; // 8 FA0D loops/sec
        size_t valsToAdd = seqdiff * valsPerSeqNum;
        datapoints.resize( datapoints.size( ) + valsToAdd, SignalData::MISSING_VALUE );

        Log::trace( ) << "; after filling: " << datapoints.size( ) << std::endl;
      }
      for ( unsigned short i = 1; i <= seqdiff; i++ ) {
        sequencenums.push_back( std::make_pair( ( currseq + i ) % 0xFF, time ) );
      }

      Log::trace( ) << "new map: " << std::endl;
      for ( const auto& m : sequencenums ) {
        Log::trace( ) << "\t" << m.first << "\t" << m.second << std::endl;
      }
    }
  }

  void StpGeReader::WaveTracker::newvalues( int waveid, std::vector<int>& values ) {
    size_t valstoread = values.size( );
    if ( 0 == expectedValues.count( waveid ) ) {
      expectedValues[waveid] = valstoread * 120;
      wavevals[waveid].reserve( expectedValues[waveid] );

      // within an FA0D block, there are 15 "mini" blocks, and waves can just
      // start appearing. To keep everything in sync, we track which miniblock
      // we're in for each wave. After a wave has been seen, its miniblock
      // count is the same as all the others, but the first time it shows up,
      // we may need to "catch up" with everybody else.
      for ( auto&m : miniseen ) {
        if ( m.second > 1 ) {
          // FIXME: this -1 isn't really right...if our new signal is in fact
          // the first signal of the miniloop, then none of the other signals
          // have incremented their miniloop yet, so we wouldn't want to
          // subtract 1 here.
          //
          // in practice, this doesn't seem to be an issue
          miniseen[waveid] = m.second - 1;
          break;
        }
      }
    }

    wavevals[waveid].insert( wavevals[waveid].end( ), values.begin( ), values.end( ) );
    miniseen[waveid]++;
  }

  bool StpGeReader::WaveTracker::writable( ) const {
    return sequencenums.size( ) > 7;
  }

  bool StpGeReader::WaveTracker::empty( ) const {
    return sequencenums.empty( );
  }

  void StpGeReader::WaveTracker::flushone( SignalSet * info ) {
    if ( empty( ) ) {
      Log::debug( ) << "no wave data to flush" << std::endl;
    }

    int erasers = std::min( (int) sequencenums.size( ), 8 );
    dr_time startt = starttime( );
    for ( auto& w : wavevals ) {
      const int waveid = w.first;
      std::vector<int>& datapoints = w.second;

      if ( datapoints.size( ) > expectedValues[waveid] ) {
        Log::trace( ) << "more values than needed (" << datapoints.size( ) << "/" << expectedValues[waveid]
            << ") for waveid: " << waveid << std::endl;
      }
      if ( datapoints.size( ) < expectedValues[waveid] ) {
        Log::trace( ) << "filling in " << ( expectedValues[waveid] - datapoints.size( ) )
            << " for waveid: " << waveid << std::endl;
        datapoints.resize( expectedValues[waveid], SignalData::MISSING_VALUE );
      }

      // make sure we have at least one val above the error code limit (-32753)
      // while converting their values to a big string
      bool waveok = false;
      for ( size_t i = 0; i < expectedValues[waveid]; i++ ) {
        if ( datapoints[i]> -32753 ) {
          waveok = true;
        }
      }

      if ( waveok ) {
        bool first = false;
        auto signal = info->addWave( wavelabel( waveid, info ), &first );
        if ( first ) {
          signal->setChunkIntervalAndSampleRate( 2000, expectedValues[waveid] );
        }

        signal->add( std::make_unique<DataRow>( startt, datapoints ) );
      }

      datapoints.erase( datapoints.begin( ), datapoints.begin( ) + expectedValues[waveid] );
    }

    sequencenums.erase( sequencenums.begin( ), sequencenums.begin( ) + erasers );

    // get ready for the next flush
    if ( empty( ) ) {
      prune( );
    }
    else {
      Log::trace( ) << "still have " << sequencenums.size( ) << " seq numbers in the chute:" << std::endl;
      //      for ( auto& x : sequencenums ) {
      //        std::cout << "\tseqs: " << x.first << "\t" << x.second << std::endl;
      //      }


      // if we have >7 sequence nums in our chute, and the first 8 all have the
      // same time, then that's our time. else use our calculated time
      if ( sequencenums.size( ) > 7 ) {
        const dr_time ctime = vitalstarttime( );

        size_t count = std::count_if( sequencenums.begin( ), sequencenums.end( ),
            [ctime]( std::pair<unsigned short, dr_time> p ) {
              return (p.second == ctime );
            } );
        if ( count > 7 ) {
          mytime = ctime - 2000; // -2000 because we're about to increment it by 2000
        }
      }
    }

    mytime += 2000;
  }

  unsigned short StpGeReader::WaveTracker::currentseq( ) const {
    return ( empty( )
        ? 0
        : ( --sequencenums.end( ) )->first );
  }

  dr_time StpGeReader::WaveTracker::vitalstarttime( ) const {
    if ( empty( ) ) {
      return 0;
    }
    return sequencenums.begin( )->second;
  }

  const dr_time StpGeReader::WaveTracker::starttime( ) const {
    return mytime;
  }
  // </editor-fold>

  StpGeReader::StpGeReader( const std::string& name ) : StpReaderBase( name ), firstread( true ) {
  }

  StpGeReader::StpGeReader( const StpGeReader& orig ) : StpReaderBase( orig ), firstread( orig.firstread ) {
  }

  StpGeReader::~StpGeReader( ) {
  }

  int StpGeReader::prepare( const std::string& filename, SignalSet * data ) {
    int rslt = StpReaderBase::prepare( filename, data );
    if ( rslt != 0 ) {
      return rslt;
    }

    magiclong = std::numeric_limits<unsigned long>::max( );
    currentTime = std::numeric_limits<dr_time>::min( );
    catchup = 0;
    catchupEven = true;
    return 0;
  }

  bool StpGeReader::popToNextSegment(){
    if( isunity() ){
      // algorithm: search forward until we find a 0xC9, then check if the preceeding 13 bytes
      // are all 0x00
      auto zeros = 0;

      while ( !work.empty( ) ) {
        const unsigned int check = popUInt8( );
        if ( 0xC9 == check && zeros >= 13 ) {
          work.rewind( 14 ); // get the 13 0x00s and the 0xC9 back on the buffer
//          for ( auto i = 0; i < 10080; i++ ) {
//            auto x = popUInt8( );
//            Log::error( ) << std::setfill( '0' ) << std::setw( 2 ) << std::hex << x << " ";
//            if ( 0 == i % 32 ) {
//              Log::error( ) << std::endl;
//            }
//          }
//          Log::error( ) << std::endl;
//          work.rewind( 10080 );

          return true;
        }

        if ( 0 == check ) {
          zeros++;
        }
        else {
          zeros = 0;
        }
      }

      return false;
    }
    else { // carescape
      unsigned long check = 0;
      while ( check != magiclong && !work.empty( ) ) {
        while ( 0x7E != work.pop( ) ) {
          // skip forward until we see another 0x7E
        }
        work.rewind( ); // get that 0x7E back in the buffer
        check = popUInt64( );
      }

      return check == magiclong;
    }
  }

  ReadResult StpGeReader::fill( SignalSet * info, const ReadResult& lastrr ) {
    //output( ) << "initial reading from input stream (popped:" << work.popped( ) << ")" << std::endl;

    int cnt = readMore( );
    if ( cnt < 0 ) {
      return ReadResult::ERROR;
    }

    if ( ReadResult::END_OF_PATIENT == lastrr ) {
      info->setMeta( "Patient Name", "" ); // reset the patient name
    }

    while ( 0 != cnt ) {
      if ( work.available( ) < 1024 * 768 ) {
        // we should never come close to filling up our work buffer, so if we do,
        // then we probably have some sort of corruption happening. see if we can find
        // another segment marker, so discard until then.
        Log::warn( ) << "work buffer is too full...attempting to recover" << std::endl;

        auto popped = work.popped( );
        if ( popToNextSegment( ) ) {
          auto newpop = work.popped( );
          Log::warn( ) << "skipping to " << std::dec << newpop
              << " to avoid damaged segment at " << popped << std::endl;
          Log::warn( ) << "work buffer has " << work.available() << " bytes available"<<std::endl;
        }
        else {
          Log::error( ) << "unrecoverable file parsing problem discovered at "
              << std::dec << popped << std::endl;
          return ReadResult::ERROR;
        }
      }

      if ( ReadResult::FIRST_READ == lastrr && std::numeric_limits<unsigned long>::max( ) == magiclong ) {
        // the first 4 bytes of the file are the segment demarcator
        magiclong = popUInt64( );
        work.rewind( 4 );
        // note: GE Unity systems seem to always have a 0 for this marker
        if ( isunity( ) ) {
          Log::info( ) << "note: assuming GE Unity Carescape input" << std::endl;
        }
      }

      ChunkReadResult rslt = ChunkReadResult::OK;
      // read as many segments as we can before reading more data
      size_t segsize = 0;
      while ( workHasFullSegment( &segsize ) && ChunkReadResult::OK == rslt ) {
        auto workdata = work.popvec(segsize);
        rslt = processOneChunk( info, workdata );

        if ( ChunkReadResult::DECODE_ERROR == rslt ) {
          return ReadResult::ERROR;
        }
        else if( ChunkReadResult::OK != rslt) {
          // something happened so rewind our to our mark
          //output( ) << "rewinding to start of segment (mark: " << ( work.popped( ) - work.poppedSinceMark( ) ) << ")" << std::endl;
          work.rewindToMark( );

          // if we're ending a file, flush all the wave data we can, but don't
          // write values that should go in the next file
          while ( !wavetracker.empty( ) && wavetracker.starttime( ) <= currentTime ) {
            //output( ) << "flushing wave data for end-of-day" << std::endl;
            wavetracker.flushone( info );
          }

          return ( ChunkReadResult::ROLLOVER == rslt
              ? ReadResult::END_OF_DURATION
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

    // if we still have stuff in our work buffer, process it
    if ( !work.empty( ) ) {
      //output( ) << "still have stuff in our work buffer!" << std::endl;
      auto workdata = work.popvec(work.size());
      processOneChunk( info, workdata );

      // we're done with the file, so write all the wave data we have
      while ( !wavetracker.empty( ) ) {
        wavetracker.flushone( info );
      }
    }
    //copySaved( filler, info );
    return ReadResult::END_OF_FILE;
  }

  bool StpGeReader::isunity( ) const {
    return 0 == magiclong;
  }

  StpGeReader::ChunkReadResult StpGeReader::processOneChunk( SignalSet * info,
      const std::vector<unsigned char>& chunkbytes ) {
    // we are guaranteed to have a complete segment in the work buffer
    // and the work buffer head is pointing to the start of the segment
    work.mark( );
    size_t chunkstart = work.popped( );
    try {
      Log::trace( ) << "processing one chunk from byte " << work.popped( ) - chunkbytes.size() << std::endl;
      StpGeSegment::GEParseError parseerr;
      auto segmentindex = StpGeSegment::index(chunkbytes, false, parseerr);
      if ( StpGeSegment::GEParseError::NO_ERROR != parseerr ) {
        if( StpGeSegment::UNKNOWN_VITALSTYPE == parseerr){
          Log::warn( ) << "unknown vitals type discovered during indexing" << std::endl;
        }
        Log::warn( ) << "parsing error (" << parseerr << ") detected in file...continuing" << std::endl;
      }

      auto lasttime = currentTime;
      currentTime = segmentindex->header.time;
      Log::trace( ) << "current time for chunk: " << currentTime << std::endl;
      if ( isRollover( currentTime, info ) ) {
        return ChunkReadResult::ROLLOVER;
      }

      if ( lasttime > -1 ) {
        if ( currentTime >= ( lasttime + 30000 ) ) {
          // if we have >30s break, just restart our counting
          catchup = 0; //( currentTime - lasttime ) / 2000;
          catchupEven = true;
        }
        else if ( currentTime >= ( lasttime + 4000 ) ) {
          // if our currentTime is more than 4s from our last time,
          // then we're missing values that we can fill in with
          // redundant times later
          catchup += ( currentTime - lasttime ) / 2000;
          catchupEven = true;
        }
        else if ( currentTime > lasttime ) {
          // reset catchup if we have a "normal" time increment (1-2s)
          //catchup--;
          if ( catchup < 0 ) {
            catchup = 0;
          }
          catchupEven = true;
        }
        else if ( currentTime == lasttime ) {
          if ( catchup >= 0 && catchupEven ) {
            // if we have the same time as our last time
            // then we've used up one catchup
            catchup--;
          }
          catchupEven = !catchupEven;
        }
      }

      if ( !segmentindex->header.patient.empty( ) ) {
        if ( 0 == info->metadata( ).count( "Patient Name" ) ) {
          info->setMeta( "Patient Name", segmentindex->header.patient );
        }
        else if ( info->metadata( ).at( "Patient Name" ) != segmentindex->header.patient ) {
          //output( ) << "new patient! (was: " << info->metadata( ).at( "Patient Name" ) << "; is: " << patient << ")" << std::endl;
          return ChunkReadResult::NEW_PATIENT;
        }
      }

      if ( isMetadataOnly( ) ) {
        // this ain't pretty, but we can't get any timing information from
        // the SignalSet if we don't have any data in it...so put some dummy
        // data in there
        info->addOffset( work.popped( ), currentTime );
        return ChunkReadResult::OK;
      }

      std::set<unsigned int> seenblocks;
      for ( const auto& vblock : segmentindex->vitals ) {
        readDataBlock( chunkbytes, vblock, info );
        seenblocks.insert( vblock.hex( ) );
      }

      // waves are always ok (?)
      //ChunkReadResult rslt = readWavesBlock( info );
      if ( !this->skipwaves( ) ) {
        readWavesBlock( chunkbytes, segmentindex, info );
      }
      return ChunkReadResult::OK;
    }
    catch ( const std::runtime_error & err ) {
      Log::error( ) << err.what( ) << " (chunk started at byte: " << chunkstart << ")" << std::endl;
      return ChunkReadResult::DECODE_ERROR;
    }
  }

  StpGeReader::ChunkReadResult StpGeReader::readWavesBlock( const std::vector<unsigned char>& workdata,
      const std::unique_ptr<StpGeSegment>& index, SignalSet * info ) {
    int fa0dloop = 0;
    for( const auto& wblock : index->waves ){
      auto oldseq = wavetracker.currentseq( );
      WaveSequenceResult wavecheck = wavetracker.newseq( wblock.sequence, currentTime );
      if ( WaveSequenceResult::TIMEBREAK == wavecheck ) {
        Log::trace( ) << "time break" << std::endl;
        while ( wavetracker.writable( ) ) {
          wavetracker.flushone( info );
        }
      }
      else if ( WaveSequenceResult::DUPLICATE == wavecheck || WaveSequenceResult::SEQBREAK == wavecheck ) {
        Log::trace( ) << "wave sequence check: "
            << ( WaveSequenceResult::DUPLICATE == wavecheck ? "DUPLICATE" : "BREAK" )
            << " (old/new): " << oldseq << "/" << wavetracker.currentseq( ) << std::endl;
      }

      for ( const auto& waveindex : wblock.wavedata ) {
        if ( 0 == StpGeReader::WAVELABELS.count( waveindex.waveid ) ) {
          std::stringstream ss;
          ss << "unknown wave id/count: "
              << std::setfill( '0' ) << std::setw( 2 ) << std::hex << waveindex.waveid << " "
              << std::setfill( '0' ) << std::setw( 2 ) << std::hex << waveindex.valcount;
          std::string ex = ss.str( );
          throw std::runtime_error( ex );
        }
        else {
          std::vector<int> vals;
          for ( size_t i = 0; i < waveindex.valcount; i++ ) {
            vals.push_back( StpGeSegment::readInt2( workdata, waveindex.datastart + i * 2 ) );
          }
          wavetracker.newvalues( waveindex.waveid, vals );
        }
      }

      fa0dloop++;
    }

    if ( 8 != fa0dloop ) {
      Log::trace( ) << "got " << fa0dloop << " loops instead of 8 at pos: " << work.popped( ) << std::endl;
    }

    while ( wavetracker.writable( ) ) {
      wavetracker.flushone( info );
    }

    return ChunkReadResult::OK;
  }

  void StpGeReader::readDataBlock( const std::vector<unsigned char>& workdata, 
      const StpGeSegment::VitalsBlock& vblock, SignalSet * info ) {
    if ( Log::levelok( LogLevel::TRACE ) && !vblock.config.empty() ) {
      Log::trace( ) << "read data block for vitals: [";
      for ( const auto& cfg : vblock.config ) {
        Log::trace( ) << " " << cfg.label;
      }
      Log::trace( ) << " ] from byte " << std::dec << vblock.datastart << std::endl;
    }

    // FIXME: rework reads so they use the workdata, not work directly
    auto read = vblock.datastart;
    for ( const auto& cfg : vblock.config ) {
      if ( cfg.isskip ) {
        read += cfg.readcount;
      }
      else {
        bool okval = false;
        bool added = false;
        unsigned int readstart = read;
        if ( cfg.unsign ) {
          unsigned int val;
          if ( 1 == cfg.readcount ) {
            val = StpGeSegment::readUInt(workdata, read);
            okval = ( val != 0x80 );
            read++;
          }
          else {
            val = StpGeSegment::readUInt2(workdata, read);
            okval = ( val != 0x8000 );
            read += 2;
          }

          if ( Log::levelok( LogLevel::TRACE ) ) {
            Log::trace( ) << cfg.label << "\t0x";
            if ( 1 == cfg.readcount ) {
              Log::trace( ) << std::setfill( '0' ) << std::setw( 2 ) << std::hex << ( val & 0xFF );
            }
            else {
              unsigned short one = ( val >> 8 );
              unsigned short two = ( val & 0xFF );

              Log::trace( ) << std::hex << std::setfill( '0' ) << std::setw( 2 ) << one
                  << std::setfill( '0' ) << std::setw( 2 ) << two;
            }
            Log::trace( ) << " => " << std::dec << val << "\t(byte: " << readstart << ")" << std::endl;
          }
          //          if ( "HR" == cfg.label ) {
          //            Log::trace( ) << "HR value: " << val << " at " << currentTime << "; catchup: " << catchup << ( okval && catchupEven && catchup >= 0 ? " keeping" : " skipping" ) << std::endl;
          //          }

          if ( okval && catchupEven && catchup >= 0 ) {
            auto sig = info->addVital( cfg.label, &added );
            if ( added ) {
              sig->setChunkIntervalAndSampleRate( 2000, 1 );
              sig->setUom( cfg.uom );
            }

            if ( cfg.divBy10 ) {
              sig->add( DataRow::from( currentTime, div10s( val, cfg.divBy10 ) ) );
            }
            else {
              sig->add( std::make_unique<DataRow>( currentTime, val ) );
            }
          }
        }
        else {
          int val;
          if ( 1 == cfg.readcount ) {
            val = StpGeSegment::readInt(workdata, read);
            okval = ( val > -128 ); // compare signed ints
            read++;
          }
          else {
            val = StpGeSegment::readInt2(workdata, read);
            okval = ( val > -32767 );
            read += 2;
          }

          if ( Log::levelok( LogLevel::TRACE ) ) {
            Log::trace( ) << cfg.label << "\t0x";
            if ( 1 == cfg.readcount ) {
              Log::trace( ) << std::setfill( '0' ) << std::setw( 2 ) << std::hex << ( val & 0xFF );
            }
            else {
              // this is just for output, so I'm okay with
              // possibly-error-producing signed shift
              short one = ( val >> 8 );
              short two = ( val & 0xFF );

              Log::trace( ) << std::hex << std::setfill( '0' ) << std::setw( 2 ) << one
                  << std::setfill( '0' ) << std::setw( 2 ) << two;
            }
            Log::trace( ) << " => " << std::dec << val << "\t(byte: " << readstart << ")" << std::endl;
          }


          if ( okval && catchupEven && catchup >= 0 ) {
            auto sig = info->addVital( cfg.label, &added );
            if ( added ) {
              sig->setChunkIntervalAndSampleRate( 2000, 1 );
              sig->setUom( cfg.uom );
            }

            if ( cfg.divBy10 ) {
              sig->add( DataRow::from( currentTime, div10s( val, cfg.divBy10 ) ) );
            }
            else {
              sig->add( std::make_unique<DataRow>( currentTime, val ) );
            }
          }
        }
      }
    }
  }

  std::string StpGeReader::div10s( int val, unsigned int multiple ) {
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

  bool StpGeReader::workHasFullSegment( size_t* size ) {
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
      // it as a 6-byte string that is repeated.
      // note: for unity systems, both of these strings are empty (doesn't matter)

      std::string magic1 = popString( 6 );
      std::string magic2 = popString( 6 );

      if ( magic1 == magic2 ) {
        if ( isunity( ) ) {
          // we've gotten the 12 0x00s, but check to make sure our next two bytes
          // are 0x00C9 before we know we're starting at the segment start
          if ( 0x00C9 == popUInt16( ) ) {
            // we've started at a segment boundary, and now we're looking for
            // 12 0x00s followed by a 0x00C9 to know we have a complete segment
            // we look for the 0xC9 byte, and then check the previous 13 bytes
            // (we expect the 0xC9 byte to be much more rare than a 0x00 byte)
            // we're looking for 13 instead of 12 0x00s, because the most
            // significant byte of 0x00C9 is, well, 0x00.
            while ( true ) {
              const unsigned int check = popUInt8( );
              if ( 0xC9 == check ) {
                unsigned long pos = work.popped( ) - 1;
                //Log::trace( ) << "found possible segment marker (0xC9) at byte: " << std::dec << pos << std::endl;
                // got a C9, so check the previous 13 bytes for 0s
                bool found = true;
                for ( int i = 1; i < 14 && found; i++ ) {
                  if ( 0 != work.read( -i - 1 ) ) {
                    found = false;
                  }
                }
                if ( found ) {
                  auto length = ( work.poppedSinceMark( ) - 14 );
                  if ( Log::levelok( LogLevel::TRACE ) ) {
                    auto start = pos + 1 - length;
                    auto end = pos - 2;
                    Log::trace( ) << "found segment in bytes: [" << std::dec << start << " - " << end << "]; size: " << end - start
                        << "; markers (0x00C9) at " << pos - 1 - length << "," << pos - 1 << std::endl;
                  }
                  ok = true;
                  if ( nullptr != size ) {
                    *size = length;
                  }
                  break;
                }
              }
            }
          }
          else {
            Log::error( ) << "missing segment boundary!" << std::endl;
          }
        }
        else { // carescape
          // search ahead for the 0x7E byte, and then check again

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
      }
      else {
        Log::error()<<"what?! work doesn't start on a segment boundary?!";
        ok = false;
      }
    }
    catch ( const std::runtime_error& x ) {
      // don't care, but don't throw
    }

    work.rewindToMark( );
    return ok;
  }

  std::string StpGeReader::wavelabel( int waveid, SignalSet * info ) {
    std::string name = StpGeReader::WAVELABELS.at( waveid );

    if( 27 == waveid ){
      // if we have PA2-X, then this is a PA2 wave
      for ( auto& v : info->vitals( ) ) {
        Log::warn( ) << "27 vital:  " << v->name( ) << std::endl;
        if ( StpGeSegment::VitalsBlock::BC_AR1_D.label == v->name( ) ) {
          return "AR1";
        }
        if ( StpGeSegment::VitalsBlock::BC_ICP1.label == v->name( ) ) {
          return "ICP1";
        }
        if ( StpGeSegment::VitalsBlock::BC_CVP1.label == v->name( ) ) {
          return "CVP1";
        }
        if ( StpGeSegment::VitalsBlock::BC_PA1_D.label == v->name( ) ) {
          return "PA1";
        }
        if ( StpGeSegment::VitalsBlock::BC_FE1_D.label == v->name( ) ) {
          return "FE1";
        }
      }
    }
    if ( 28 == waveid ) {
      // if we have PA2-X, then this is a PA2 wave
      for ( auto& v : info->vitals( ) ) {
        Log::warn( ) << "28 vital:  " << v->name( ) << std::endl;
        if ( StpGeSegment::VitalsBlock::BC_AR2_D.label == v->name( ) ) {
          return "AR2";
        }
        if ( StpGeSegment::VitalsBlock::BC_ICP2.label == v->name( ) ) {
          return "ICP2";
        }
        if ( StpGeSegment::VitalsBlock::BC_CVP2.label == v->name( ) ) {
          return "CVP2";
        }
        if ( StpGeSegment::VitalsBlock::BC_PA2_D.label == v->name( ) ) {
          return "PA2";
        }
        if ( StpGeSegment::VitalsBlock::BC_FE2_D.label == v->name( ) ) {
          return "FE2";
        }
      }
    }
    else if ( 29 == waveid ) {
      for ( auto& v : info->vitals( ) ) {
        if ( StpGeSegment::VitalsBlock::BC_AR3_D.label == v->name( ) ) {
          return "AR3";
        }
        if ( StpGeSegment::VitalsBlock::BC_ICP3.label == v->name( ) ) {
          return "ICP3";
        }
        if ( StpGeSegment::VitalsBlock::BC_CVP3.label == v->name( ) ) {
          return "CVP3";
        }
        if ( StpGeSegment::VitalsBlock::BC_PA3_D.label == v->name( ) ) {
          return "PA3";
        }
        if ( StpGeSegment::VitalsBlock::BC_FE3_D.label == v->name( ) ) {
          return "FE3";
        }
      }
    }
    else if ( 30 == waveid ) {
      for ( auto& v : info->vitals( ) ) {
        if ( StpGeSegment::VitalsBlock::BC_AR4_D.label == v->name( ) ) {
          return "AR4";
        }
        if ( StpGeSegment::VitalsBlock::BC_ICP4.label == v->name( ) ) {
          return "ICP4";
        }
        if ( StpGeSegment::VitalsBlock::BC_CVP4.label == v->name( ) ) {
          return "CVP4";
        }
        if ( StpGeSegment::VitalsBlock::BC_PA4_D.label == v->name( ) ) {
          return "PA4";
        }
        if ( StpGeSegment::VitalsBlock::BC_FE4_D.label == v->name( ) ) {
          return "FE4";
        }
      }
    }

    return name;
  }

  std::vector<StpReaderBase::StpMetadata> StpGeReader::parseMetadata( const std::string& input ) {
    std::vector<StpReaderBase::StpMetadata> metas;
    auto info = std::unique_ptr<SignalSet>{ std::make_unique<BasicSignalSet>( ) };
    StpGeReader reader;
    reader.splitter( SplitLogic::nobreaks( ) );
    int failed = reader.prepare( input, info.get( ) );
    if ( failed ) {
      Log::error( ) << "error while opening input file. error code: " << failed << std::endl;
      return metas;
    }
    reader.setMetadataOnly( );

    ReadResult last = ReadResult::FIRST_READ;

    bool okToContinue = true;
    while ( okToContinue ) {
      last = reader.fill( info.get( ), last );
      switch ( last ) {
        case ReadResult::FIRST_READ:
          // NOTE: no break here
        case ReadResult::NORMAL:
          break;
        case ReadResult::END_OF_DURATION:
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
          Log::error( ) << "error while reading input file" << std::endl;
          okToContinue = false;
          break;
      }
    }

    reader.finish( );

    return metas;
  }

  StpReaderBase::StpMetadata StpGeReader::metaFromSignalSet( const std::unique_ptr<SignalSet>& info ) {
    StpReaderBase::StpMetadata meta;
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