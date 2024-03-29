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
    {0x1B, "AR1" }, // or ICP, or PA, or FE...
    {0x1C, "AR2" }, // or ICP, or PA, or FE...
    {0x1D, "AR3" }, // or ICP, or PA, or FE...
    {0x1E, "AR4" }, // or ICP, or PA, or FE...
    {0x27, "SPO2" },
    {0x2A, "CO2" },
    {0xC8, "VNT_PRES" },
    {0xC9, "VNT_FLOW" },
  };

  // <editor-fold defaultstate="collapsed" desc="Wave Tracker">

  StpGeReader::WaveTracker::SequenceData::SequenceData( unsigned short seq, dr_time t )
      : sequence( seq ), time( t ) {
  }

  StpGeReader::WaveTracker::SequenceData::SequenceData( const SequenceData& other )
      : sequence( other.sequence ), time( other.time ) {
    for ( const auto& x : other.wavedata ) {
      wavedata.insert({ x.first, std::vector<int>( x.second.begin( ), x.second.end( ) ) } );
    }
  }

  StpGeReader::WaveTracker::SequenceData& StpGeReader::WaveTracker::SequenceData::operator=(const SequenceData& other ) {
    if ( this != &other ) {
      this->sequence = other.sequence;
      this->time = other.time;
      wavedata.clear( );
      for ( const auto& x : other.wavedata ) {
        wavedata.insert({ x.first, std::vector<int>( x.second.begin( ), x.second.end( ) ) } );
      }
    }

    return *this;
  }

  StpGeReader::WaveTracker::SequenceData::~SequenceData( ) { }

  void StpGeReader::WaveTracker::SequenceData::push( int waveid, const std::vector<int>& data ) {
    if ( 0 == wavedata.count( waveid ) ) {
      wavedata.insert({ waveid, std::vector<int>{} } );
    }

    wavedata.at(waveid).insert( wavedata.at(waveid).end( ), data.begin( ), data.end( ) );
  }

  StpGeReader::WaveTracker::WaveTracker( ) {
  }

  StpGeReader::WaveTracker::~WaveTracker( ) {
  }

  void StpGeReader::WaveTracker::prune( ) {
  }

  void StpGeReader::WaveTracker::filterDuplicates(){
    // filter outs sequences that contain duplicate data
    // this is harder that you think because a waveform might be a constant number
    // which would look like a endless duplicate...we will inspect all the waveforms
    // values in a given sequence, and if we find *any* duplicate waveform, we'll 
    // assume the whole sequence is duplicated. To help avoid false positives, we'll
    // only check waveforms that are not a single repeated number
    if( sequencedata.size()<2){
      return;
    }

    std::map<int, std::vector<int>> lastvals;
    auto first = true;
    auto removers=std::set<unsigned short>{};
    for( const auto& seqdat: sequencedata){
      auto isdupe = false;
      if( first ){
        // skip the first element, since it obviously 
        // can't be a duplicate (nothing to compare it to)
        first = false;
      }
      else if ( !isdupe ) {
        // check the current values vs the last values
        for ( const auto& needle : seqdat.wavedata ) {
          if ( !isdupe && !std::all_of( needle.second.begin( ), needle.second.end( ),
              [haystack = needle.second](int i){ return haystack.at( 0 ) == i; } ) ) {
            // not all one number, so check this value

            if ( 1 == lastvals.count( needle.first ) && lastvals.at( needle.first ) == needle.second ) {
              if ( !dupewarned ) {
                Log::warn( ) << "Duplicate waveform detected" << std::endl;
                dupewarned = true;
              }

              Log::trace( ) << "Duplicate waveform (0x" <<
                  std::setw( 2 ) << std::setfill( '0' ) << std::hex
                  << needle.first << ") detected at sequence id: "
                  << seqdat.sequence << "...skipping whole sequence" << std::endl;
              isdupe = true;
              removers.insert( seqdat.sequence );
            }
          }
        }
      }
      lastvals = seqdat.wavedata;
    }

    if(!removers.empty()){
      sequencedata.erase(std::remove_if( sequencedata.begin(), sequencedata.end(), 
        [removers](auto vec){return removers.count(vec.sequence)>0;}),
        sequencedata.end());
    }
  }

  StpGeReader::WaveSequenceResult StpGeReader::WaveTracker::newseq( const unsigned short& seqnum, dr_time time ) {
    WaveSequenceResult rslt = WaveSequenceResult::NORMAL;
    if ( empty( ) ) {
      prune( );
      Log::trace( ) << "starttrack " << std::dec << time << " (-): " << std::setw( 2 )
          << std::setfill( '0' ) << std::hex << seqnum << std::dec << std::endl;
    }
    else {
      unsigned short currseq = currentseq( );
      const auto lasttime = (--sequencedata.end())->time;
      auto timediff = time - lasttime;

      Log::trace( ) << "new seq at " << std::dec << time << " (" << timediff << "): "
          << std::setw( 2 ) << std::setfill( '0' ) << std::hex << seqnum << std::dec << std::endl;

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

          if ( timediff >= 4000 ) { // 4s is arbitrary, but we commonly see 1s & 3s breaks
            // sequence is right, but the time is off, so figure out what
            // the time should be by reading what's in our chute
            size_t counter = 0;
            auto it = sequencedata.rbegin( );
            dr_time xtime = it->time;
            while ( it++ != sequencedata.rend( ) && ++counter < 8 && it->time == xtime ) {
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
        breaksync( rslt, currseq, seqnum, lasttime, time );
      }
    }

    sequencedata.push_back( SequenceData{ seqnum, time } );
    miniseen.clear();
    return rslt;
  }

  void StpGeReader::WaveTracker::resetDupeWarn( ) {
    dupewarned = false;
  }

  void StpGeReader::WaveTracker::breaksync( StpGeReader::WaveSequenceResult rslt,
      const unsigned short& currseq, const unsigned short& newseq, const dr_time& currtime,
      const dr_time& newtime ) {

    // for timed breaks, we really just want to fill up the last second's worth of missing data

    // for sequence breaks, we don't want to fill up the whole second...just
    // the missing sequences.

    // if we're missing >8 sequence numbers, treat it as if it's a timed break

    // the -1 is because we haven't read the seqnum values yet
    // we don't use these variables if it's a timedbreak
    unsigned short seqcalc = ( newseq - 1 < currseq ? newseq + 0xFF : newseq - 1 );
    unsigned short seqdiff = ( seqcalc - currseq );

    const auto dotimed = currtime != newtime;
    // if our current time differs from our new time, look in the sequence data to figure out
    // how many sequence numbers we need to fill up the current second
    auto addloops = dotimed
        // 8 FA0D loops/sec, and we want to fill in the last second
        ? 8 - std::count_if( sequencedata.begin( ), sequencedata.end( ),
        [currtime](const auto& p ) {
          return (p.time == currtime );
        } )
    // seconds are the same, but we need extra sequence numbers
    : seqdiff;

    Log::debug( ) << "(break) filling in " << std::dec << addloops << " seqnums at time: "
        << currtime << std::endl;
    for ( auto lim = 1; lim <= addloops; lim++ ) {
      // add the missing sequence number
      auto seqd = SequenceData{ (unsigned short) ( ( currseq + lim ) % 0xFF ), currtime };

      // fill in the missing values for each wave
      for ( const auto& x : ( --sequencedata.end( ) )->wavedata ) {
        size_t valsPerSeqNum = expectedValues[x.first] / 8; // 8 FA0D loops/sec
        seqd.push( x.first, std::vector<int>( valsPerSeqNum, SignalData::MISSING_VALUE ) );
      }
      sequencedata.push_back( seqd );
    }

    Log::trace( ) << "new map: " << std::endl;
    for ( const auto& m : sequencedata ) {
      Log::trace( ) << "\t" << std::setw( 2 ) << std::setfill( '0' ) << std::hex <<
          m.sequence << "\t" << std::dec << m.time << std::endl;
    }
  }

  void StpGeReader::WaveTracker::newvalues( int waveid, std::vector<int>& values ) {
    // for( const auto& x: sequencedata) {
    //   Log::debug( ) << "sequencedata: " << x.sequence << " " << x.time << " "
    //       << x.wavedata.size( ) << " " << std::hex << &( x.wavedata ) << std::endl;
    // }
    if( Log::levelok(LogLevel::TRACE)){
      Log::trace( ) << "adding new values to wavetracker/seq::"
       << std::setw( 2 ) << std::setfill( '0' ) << std::hex  << this->currentseq() << " "
       << std::dec<<waveid<<"(0x" << std::setw( 2 ) << std::setfill( '0' ) << std::hex << waveid << ")";
      for( const auto& x : values){
        Log::trace()<<" "<<std::dec<<x;
      }
      Log::trace()<<std::endl;
    }

    size_t valstoread = values.size( );
    auto& seqdata = (--sequencedata.end());
    if ( 0 == expectedValues.count( waveid ) ) {
      expectedValues[waveid] = valstoread * 120;

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

    // FIXME: why do I have to assign this again here?
    seqdata = (--sequencedata.end());
    seqdata->push( waveid, values );
    miniseen[waveid]++;
  }

  bool StpGeReader::WaveTracker::writable( ) const {
    return sequencedata.size( ) > 7;
  }

  bool StpGeReader::WaveTracker::empty( ) const {
    return sequencedata.empty( );
  }

  void StpGeReader::WaveTracker::flushone( SignalSet * info ) {
    Log::trace( ) << "flush sequence " << std::dec << starttime( ) << std::endl;
    if ( empty( ) ) {
      Log::debug( ) << "no wave data to flush" << std::endl;
    }

    // NOTE: there are 4 sequence ids/second, so we expect 8 sequencedata entries for a 2s block.
    // Of course, we don't always get that many (any sometimes get many more)

    const auto startt = starttime();
    int erasers = std::min( (int) sequencedata.size( ), 8 );
    auto loopcnt = 0;
    auto wavesdata = std::map<int, std::vector<int>>{};
    while ( loopcnt < erasers ) {
      auto sd = sequencedata.begin( ) + loopcnt;
      for ( auto& w : sd->wavedata ) {
        const int waveid = w.first;
        std::vector<int>& datapoints = w.second;
        if ( 0 == wavesdata.count( waveid ) ) {
          wavesdata.insert({ waveid, std::vector<int>() } );
        }
        auto& datavec = wavesdata.at( waveid );
        datavec.insert( datavec.end( ), datapoints.begin( ), datapoints.end( ) );
      }

      loopcnt++;
    }

    for ( auto& wd : wavesdata ) {
      auto waveid = wd.first;
      auto& datavec = wd.second;

      if ( datavec.size( ) > expectedValues[waveid] ) {
        Log::trace( ) << "more values than needed (" << std::dec << datavec.size( )
            << "/" << expectedValues[waveid] << ") for waveid: " << waveid << std::endl;
      }
      if ( datavec.size( ) < expectedValues[waveid] ) {
        Log::trace( ) << "filling in " << std::dec << ( expectedValues[waveid] - datavec.size( ) )
            << " for waveid: " << waveid << std::endl;
        datavec.resize( expectedValues[waveid], SignalData::MISSING_VALUE );
      }

      // make sure we have at least one val above the error code limit (-32753)
      bool waveok = false;
      for ( size_t i = 0; i < expectedValues[waveid]; i++ ) {
        if ( datavec[i]> -32753 ) {
          waveok = true;
        }
      }

      if ( waveok ) {
        bool first = false;
        auto signal = info->addWave( wavelabel( waveid, info ), &first );
        if ( first ) {
          signal->setChunkIntervalAndSampleRate( 2000, expectedValues[waveid] );
        }

        signal->add( std::make_unique<DataRow>( startt, datavec ) );
      }
    }

    sequencedata.erase( sequencedata.begin( ), sequencedata.begin( ) + erasers );

    // get ready for the next flush
    if ( empty( ) ) {
      prune( );
    }
    else {
      Log::trace( ) << "still have " << sequencedata.size( ) << " seq numbers in the chute:" << std::endl;
      for ( const auto& m : sequencedata ) {
        Log::trace( ) << "\t" << std::setw( 2 ) << std::setfill( '0' ) << std::hex <<
            m.sequence << "\t" << std::dec << m.time << std::endl;
      }
    }
  }

  unsigned short StpGeReader::WaveTracker::currentseq( ) const {
    return ( empty( )
        ? 0
        : ( --sequencedata.end( ) )->sequence );
  }

  dr_time StpGeReader::WaveTracker::vitalstarttime( ) const {
    if ( empty( ) ) {
      return 0;
    }
    return sequencedata.begin( )->time;
  }

  const dr_time StpGeReader::WaveTracker::starttime( ) const {
    return vitalstarttime();
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
    wavetracker.resetDupeWarn();
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

          if( newpop == popped ){
            // popped to the same place?!
            Log::error( ) << "file parsing loop discovered at " << std::dec << popped << std::endl;
            return ReadResult::ERROR;
          }

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
        else if ( ChunkReadResult::SKIP == rslt ) {
          // skip this chunk, but process the next one
          return ReadResult::NORMAL;
        }
        else if( ChunkReadResult::OK != rslt) {
          // something happened so rewind our to our mark
          //output( ) << "rewinding to start of segment (mark: " << ( work.popped( ) - work.poppedSinceMark( ) ) << ")" << std::endl;
          work.rewindToMark( );

          // if we're ending a file, flush all the wave data we can, but don't
          // write values that should go in the next file
          wavetracker.filterDuplicates();
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
      wavetracker.filterDuplicates();
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
      Log::trace( ) << "current time for chunk: " << std::dec << currentTime << std::endl;

      if( !isUsableDate(currentTime)){
        return ChunkReadResult::SKIP;
      }

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
        wavetracker.filterDuplicates();
        while ( wavetracker.writable( ) ) {
          wavetracker.flushone( info );
        }
      }
      else if ( WaveSequenceResult::DUPLICATE == wavecheck || WaveSequenceResult::SEQBREAK == wavecheck ) {
        Log::trace( ) << "wave sequence check: "
            << ( WaveSequenceResult::DUPLICATE == wavecheck ? "DUPLICATE" : "BREAK" )
            << " (old/new): " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << oldseq
            << "/" << std::setfill( '0' ) << std::setw( 2 ) << std::hex << wavetracker.currentseq( )
            << std::endl;
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

    wavetracker.filterDuplicates();
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
        //Log::warn( ) << "27 vital:  " << v->name( ) << std::endl;
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