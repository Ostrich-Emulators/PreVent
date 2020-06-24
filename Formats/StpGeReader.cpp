/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "StpGeReader.h"
#include "SignalData.h"
#include "DataRow.h"
#include "BasicSignalSet.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <filesystem>
#include "config.h"
#include "CircularBuffer.h"

namespace FormatConverter{

  /**
   * Note: wave labels *can* change depending on what vitals are in the file
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
    {0x1B, "AR1" },
    {0x1C, "ICP2" }, // may also be PA2
    {0x1D, "CVP3" }, // may also be PA3
    {0x1E, "CVP4" },
    {0x27, "SPO2" },
    {0x2A, "CO2" },
    {0xC8, "VNT_PRES" },
    {0xC9, "VNT_FLOW" },
  };

  // <editor-fold defaultstate="collapsed" desc="block configs">
  const StpGeReader::BlockConfig StpGeReader::SKIP = BlockConfig::skip( );
  const StpGeReader::BlockConfig StpGeReader::SKIP2 = BlockConfig::skip( 2 );
  const StpGeReader::BlockConfig StpGeReader::SKIP4 = BlockConfig::skip( 4 );
  const StpGeReader::BlockConfig StpGeReader::SKIP5 = BlockConfig::skip( 5 );
  const StpGeReader::BlockConfig StpGeReader::SKIP6 = BlockConfig::skip( 6 );
  const StpGeReader::BlockConfig StpGeReader::HR = BlockConfig::vital( "HR", "Bpm" );
  const StpGeReader::BlockConfig StpGeReader::PVC = BlockConfig::vital( "PVC", "Bpm" );
  const StpGeReader::BlockConfig StpGeReader::STI = BlockConfig::div10( "ST-I", "mm", 1, false );
  const StpGeReader::BlockConfig StpGeReader::STII = BlockConfig::div10( "ST-II", "mm", 1, false );
  const StpGeReader::BlockConfig StpGeReader::STIII = BlockConfig::div10( "ST-III", "mm", 1, false );
  const StpGeReader::BlockConfig StpGeReader::STAVR = BlockConfig::div10( "ST-AVR", "mm", 1, false );
  const StpGeReader::BlockConfig StpGeReader::STAVL = BlockConfig::div10( "ST-AVL", "mm", 1, false );
  const StpGeReader::BlockConfig StpGeReader::STAVF = BlockConfig::div10( "ST-AVF", "mm", 1, false );
  const StpGeReader::BlockConfig StpGeReader::STV = BlockConfig::div10( "ST-V", "mm", 1, false );
  const StpGeReader::BlockConfig StpGeReader::STV1 = BlockConfig::div10( "ST-V1", "mm", 1, false );
  const StpGeReader::BlockConfig StpGeReader::BT = BlockConfig::div10( "BT", "Deg C", 2 );
  const StpGeReader::BlockConfig StpGeReader::IT = BlockConfig::div10( "IT", "Deg C", 2 );
  const StpGeReader::BlockConfig StpGeReader::RESP = BlockConfig::vital( "RESP", "BrMin" );
  const StpGeReader::BlockConfig StpGeReader::APNEA = BlockConfig::vital( "APNEA", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::NBP_M = BlockConfig::vital( "NBP-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::NBP_S = BlockConfig::vital( "NBP-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::NBP_D = BlockConfig::vital( "NBP-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CUFF = BlockConfig::vital( "CUFF", "mmHg", 2, false );
  const StpGeReader::BlockConfig StpGeReader::AR1_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR1_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR1_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR1_R = BlockConfig::vital( "AR1-R", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR2_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR2_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR2_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR2_R = BlockConfig::vital( "AR1-R", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR3_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR3_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR3_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR3_R = BlockConfig::vital( "AR1-R", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR4_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR4_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR4_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::AR4_R = BlockConfig::vital( "AR1-R", "mmHg" );

  const StpGeReader::BlockConfig StpGeReader::SPO2_P = BlockConfig::vital( "SPO2-%", "%" );
  const StpGeReader::BlockConfig StpGeReader::SPO2_R = BlockConfig::vital( "SPO2-R", "Bpm" );
  const StpGeReader::BlockConfig StpGeReader::VENT = BlockConfig::vital( "Vent Rate", "BrMin" );
  const StpGeReader::BlockConfig StpGeReader::IN_HLD = BlockConfig::div10( "IN_HLD", "Sec", 2, false );
  const StpGeReader::BlockConfig StpGeReader::PRS_SUP = BlockConfig::vital( "PRS-SUP", "cmH2O" );
  const StpGeReader::BlockConfig StpGeReader::INSP_TM = BlockConfig::div100( "INSP-TM", "Sec", 2, false );
  const StpGeReader::BlockConfig StpGeReader::INSP_PC = BlockConfig::vital( "INSP-PC", "%" );
  const StpGeReader::BlockConfig StpGeReader::I_E = BlockConfig::div10( "I:E", "", 2, false );
  const StpGeReader::BlockConfig StpGeReader::SET_PCP = BlockConfig::vital( "SET-PCP", "cmH2O" );
  const StpGeReader::BlockConfig StpGeReader::SET_IE = BlockConfig::div10( "SET-IE", "", 2, false );
  const StpGeReader::BlockConfig StpGeReader::APRV_LO_T = BlockConfig::div10( "APRV-LO-T", "Sec", 2, false );
  const StpGeReader::BlockConfig StpGeReader::APRV_HI_T = BlockConfig::div10( "APRV-HI-T", "Sec", 2, false );
  const StpGeReader::BlockConfig StpGeReader::APRV_LO = BlockConfig::vital( "APRV-LO", "cmH20", 2, false );
  const StpGeReader::BlockConfig StpGeReader::APRV_HI = BlockConfig::vital( "APRV-HI", "cmH20", 2, false );
  const StpGeReader::BlockConfig StpGeReader::RESIS = BlockConfig::div10( "RESIS", "cmH2O/L/Sec", 2, false );
  const StpGeReader::BlockConfig StpGeReader::MEAS_PEEP = BlockConfig::vital( "MEAS-PEEP", "cmH2O" );
  const StpGeReader::BlockConfig StpGeReader::INTR_PEEP = BlockConfig::vital( "INTR-PEEP", "cmH2O", 2, false );
  const StpGeReader::BlockConfig StpGeReader::INSP_TV = BlockConfig::vital( "INSP-TV", "" );
  const StpGeReader::BlockConfig StpGeReader::COMP = BlockConfig::vital( "COMP", "ml/cmH20" );

  const StpGeReader::BlockConfig StpGeReader::SPONT_MV = BlockConfig::vital( "SPONT-MV", "L/Min", 2, false );
  const StpGeReader::BlockConfig StpGeReader::SPONT_R = BlockConfig::vital( "SPONT-R", "BrMin", 2, false );
  const StpGeReader::BlockConfig StpGeReader::SET_TV = BlockConfig::vital( "SET-TV", "ml", 2, false );
  const StpGeReader::BlockConfig StpGeReader::B_FLW = BlockConfig::vital( "B-FLW", "L/Min", 2, false );
  const StpGeReader::BlockConfig StpGeReader::FLW_R = BlockConfig::vital( "FLW-R", "cmH20", 2, false );
  const StpGeReader::BlockConfig StpGeReader::FLW_TRIG = BlockConfig::vital( "FLW-TRIG", "L/Min", 2, false );
  const StpGeReader::BlockConfig StpGeReader::HF_FLW = BlockConfig::vital( "HF-FLW", "L/Min", 2, false );
  const StpGeReader::BlockConfig StpGeReader::HF_R = BlockConfig::vital( "HF-R", "Sec", 2, false );
  const StpGeReader::BlockConfig StpGeReader::HF_PRS = BlockConfig::vital( "HF-PRS", "cmH2O", 2, false );
  const StpGeReader::BlockConfig StpGeReader::TMP_1 = BlockConfig::div10( "TMP-1", "Deg C" );
  const StpGeReader::BlockConfig StpGeReader::TMP_2 = BlockConfig::div10( "TMP-2", "Deg C" );
  const StpGeReader::BlockConfig StpGeReader::DELTA_TMP = BlockConfig::div10( "DELTA-TMP", "Deg C" );
  const StpGeReader::BlockConfig StpGeReader::LA1 = BlockConfig::vital( "LA1", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CVP1 = BlockConfig::vital( "CVP1", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CVP2 = BlockConfig::vital( "CVP2", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CVP3 = BlockConfig::vital( "CVP3", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CVP4 = BlockConfig::vital( "CVP4", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CPP1 = BlockConfig::vital( "CPP1", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::ICP1 = BlockConfig::vital( "ICP1", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CPP2 = BlockConfig::vital( "CPP2", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::ICP2 = BlockConfig::vital( "ICP2", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CPP3 = BlockConfig::vital( "CPP3", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::ICP3 = BlockConfig::vital( "ICP3", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CPP4 = BlockConfig::vital( "CPP4", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::ICP4 = BlockConfig::vital( "ICP4", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::SP1 = BlockConfig::vital( "SP1", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA1_S = BlockConfig::vital( "PA1-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA1_D = BlockConfig::vital( "PA1-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA1_R = BlockConfig::vital( "PA1-R", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA1_M = BlockConfig::vital( "PA1-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA2_S = BlockConfig::vital( "PA2-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA2_D = BlockConfig::vital( "PA2-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA2_R = BlockConfig::vital( "PA2-R", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA2_M = BlockConfig::vital( "PA2-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA3_S = BlockConfig::vital( "PA3-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA3_D = BlockConfig::vital( "PA3-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA3_R = BlockConfig::vital( "PA3-R", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA3_M = BlockConfig::vital( "PA3-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA4_S = BlockConfig::vital( "PA4-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA4_D = BlockConfig::vital( "PA4-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA4_R = BlockConfig::vital( "PA4-R", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::PA4_M = BlockConfig::vital( "PA4-M", "mmHg" );

  const StpGeReader::BlockConfig StpGeReader::UAC1_S = BlockConfig::vital( "UAC1-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC1_D = BlockConfig::vital( "UAC1-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC1_R = BlockConfig::vital( "UAC1-R", "Bpm" );
  const StpGeReader::BlockConfig StpGeReader::UAC1_M = BlockConfig::vital( "UAC1-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC2_S = BlockConfig::vital( "UAC2-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC2_D = BlockConfig::vital( "UAC2-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC2_R = BlockConfig::vital( "UAC2-R", "Bpm" );
  const StpGeReader::BlockConfig StpGeReader::UAC2_M = BlockConfig::vital( "UAC2-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC3_S = BlockConfig::vital( "UAC3-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC3_D = BlockConfig::vital( "UAC3-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC3_R = BlockConfig::vital( "UAC3-R", "Bpm" );
  const StpGeReader::BlockConfig StpGeReader::UAC3_M = BlockConfig::vital( "UAC3-M", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC4_S = BlockConfig::vital( "UAC4-S", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC4_D = BlockConfig::vital( "UAC4-D", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::UAC4_R = BlockConfig::vital( "UAC4-R", "Bpm" );
  const StpGeReader::BlockConfig StpGeReader::UAC4_M = BlockConfig::vital( "UAC4-M", "mmHg" );

  const StpGeReader::BlockConfig StpGeReader::PT_RR = BlockConfig::vital( "PT-RR", "BrMin" );
  const StpGeReader::BlockConfig StpGeReader::PEEP = BlockConfig::vital( "PEEP", "cmH20" );
  const StpGeReader::BlockConfig StpGeReader::MV = BlockConfig::div10( "MV", "L/min" );
  const StpGeReader::BlockConfig StpGeReader::Fi02 = BlockConfig::vital( "Fi02", "%" );
  const StpGeReader::BlockConfig StpGeReader::TV = BlockConfig::vital( "TV", "ml" );
  const StpGeReader::BlockConfig StpGeReader::PIP = BlockConfig::vital( "PIP", "cmH20" );
  const StpGeReader::BlockConfig StpGeReader::PPLAT = BlockConfig::vital( "PPLAT", "cmH20" );
  const StpGeReader::BlockConfig StpGeReader::MAWP = BlockConfig::vital( "MAWP", "cmH20" );
  const StpGeReader::BlockConfig StpGeReader::SENS = BlockConfig::div10( "SENS", "cmH20" );

  const StpGeReader::BlockConfig StpGeReader::CO2_EX = BlockConfig::vital( "CO2-EX", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CO2_IN = BlockConfig::vital( "CO2-IN", "mmHg" );
  const StpGeReader::BlockConfig StpGeReader::CO2_RR = BlockConfig::vital( "CO2-RR", "BrMin" );
  const StpGeReader::BlockConfig StpGeReader::O2_EXP = BlockConfig::div10( "O2-EXP", "%" );
  const StpGeReader::BlockConfig StpGeReader::O2_INSP = BlockConfig::div10( "O2-INSP", "%" ); // </editor-fold>

  // <editor-fold defaultstate="collapsed" desc="Wave Tracker">

  StpGeReader::WaveTracker::WaveTracker( ) { }

  StpGeReader::WaveTracker::~WaveTracker( ) { }

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
      std::cerr << "NO sequence numbers, but wave vals? WRONG!" << std::endl;
      for ( const auto& m : wavevals ) {
        std::cerr << "\twave: " << m.first << "\t" << m.second.size( ) << std::endl;
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
        std::cout << "wave " << waveid << ": before filling: " << datapoints.size( );
        // break is based on missing sequence numbers
        size_t valsPerSeqNum = expectedValues[waveid] / 8; // 8 FA0D loops/sec
        size_t valsToAdd = loops * valsPerSeqNum;
        datapoints.resize( datapoints.size( ) + valsToAdd, SignalData::MISSING_VALUE );

        std::cout << "; after filling: " << datapoints.size( ) << std::endl;
      }
    }
    else {

      std::cout << "small break in sequence (" << seqdiff << " elements) current map:" << std::endl;
      for ( const auto& m : sequencenums ) {
        std::cout << "\t" << m.first << "\t" << m.second << std::endl;
      }

      // we have a small sequence difference, so fill in phantom values
      for ( auto& w : wavevals ) {
        const int waveid = w.first;
        std::vector<int>& datapoints = w.second;

        std::cout << "wave " << waveid << ": before filling: " << datapoints.size( );
        // break is based on missing sequence numbers
        size_t valsPerSeqNum = expectedValues[waveid] / 8; // 8 FA0D loops/sec
        size_t valsToAdd = seqdiff * valsPerSeqNum;
        datapoints.resize( datapoints.size( ) + valsToAdd, SignalData::MISSING_VALUE );

        std::cout << "; after filling: " << datapoints.size( ) << std::endl;
      }
      for ( unsigned short i = 1; i <= seqdiff; i++ ) {
        sequencenums.push_back( std::make_pair( ( currseq + i ) % 0xFF, time ) );
      }

      std::cout << "new map: " << std::endl;
      for ( const auto& m : sequencenums ) {
        std::cout << "\t" << m.first << "\t" << m.second << std::endl;
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
      std::cout << "no wave data to flush" << std::endl;
    }

    int erasers = std::min( (int) sequencenums.size( ), 8 );
    //    if ( mytime >= 1475072107000 ) {
    //      std::cout << "flushing " << erasers << " values from seqnum " << sequencenums[0].first << "\t"
    //              << sequencenums[0].second << " (" << mytime << ")...";
    //
    //    }

    dr_time startt = starttime( );
    for ( auto& w : wavevals ) {
      const int waveid = w.first;
      std::vector<int>& datapoints = w.second;

      if ( datapoints.size( ) > expectedValues[waveid] ) {
        std::cout << "more values than needed (" << datapoints.size( ) << "/" << expectedValues[waveid]
            << ") for waveid: " << waveid << std::endl;
      }
      if ( datapoints.size( ) < expectedValues[waveid] ) {
        std::cout << "filling in " << ( expectedValues[waveid] - datapoints.size( ) )
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

      if ( waveok && !this->skipwaves() ) {
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
      std::cout << "still have " << sequencenums.size( ) << " seq numbers in the chute:" << std::endl;
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
    //    if ( mytime >= 1475072107000 ) {
    //      std::cout << "done" << std::endl;
    //    }
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

  StpGeReader::StpGeReader( const std::string& name ) : StpReaderBase( name ), firstread( true ) { }

  StpGeReader::StpGeReader( const StpGeReader& orig ) : StpReaderBase( orig ), firstread( orig.firstread ) { }

  StpGeReader::~StpGeReader( ) { }

  int StpGeReader::prepare( const std::string& filename, SignalSet * data ) {
    int rslt = StpReaderBase::prepare( filename, data );
    if ( rslt != 0 ) {
      return rslt;
    }

    magiclong = std::numeric_limits<unsigned long>::max( );
    currentTime = 0;
    return 0;
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
        // we should never come close to filling up our work buffer
        // so if we have, make sure the sure knows
        std::cerr << "work buffer is too full...something is going wrong" << std::endl;
        return ReadResult::ERROR;
      }

      if ( ReadResult::FIRST_READ == lastrr && std::numeric_limits<unsigned long>::max( ) == magiclong ) {
        // the first 4 bytes of the file are the segment demarcator
        magiclong = popUInt64( );
        work.rewind( 4 );
        // note: GE Unity systems seem to always have a 0 for this marker
        if ( isunity( ) ) {
          output( ) << "note: assuming GE Unity Carescape input" << std::endl;
        }
      }

      ChunkReadResult rslt = ChunkReadResult::OK;
      // read as many segments as we can before reading more data
      size_t segsize = 0;
      while ( workHasFullSegment( &segsize ) && ChunkReadResult::OK == rslt ) {
        //output( ) << "next segment is " << std::dec << segsize << " bytes big" << std::endl;

        size_t startpop = work.popped( );
        rslt = processOneChunk( info, segsize );
        size_t endpop = work.popped( );

        if ( ChunkReadResult::OK == rslt ) {
          size_t bytesread = endpop - startpop;
          //output( ) << "read " << bytesread << " bytes of segment" << std::endl;
          if ( bytesread < segsize ) {
            work.skip( segsize - bytesread );
            //output( ) << "skipping ahead " << ( segsize - bytesread ) << " bytes to next segment at " << ( startpop + segsize ) << std::endl;
          }
        }
        else if ( ChunkReadResult::UNKNOWN_BLOCKTYPE == rslt ) {
          return ReadResult::ERROR;
        }
        else {
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
              ? ReadResult::END_OF_DAY
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

    //output( ) << "file is exhausted" << std::endl;

    // copy any data we have left in our filler set to the real set

    // if we still have stuff in our work buffer, process it
    if ( !work.empty( ) ) {
      //output( ) << "still have stuff in our work buffer!" << std::endl;
      processOneChunk( info, work.size( ) );

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
      const size_t& maxread ) {
    // we are guaranteed to have a complete segment in the work buffer
    // and the work buffer head is pointing to the start of the segment
    work.mark( );
    size_t chunkstart = work.popped( );
    //output( ) << "processing one chunk from byte " << work.popped( ) << std::endl;
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
          unsigned int blocktypefmt = popUInt16( );
          work.rewind( 68 ); // go back to the start of this block
          //output( ) << "new block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
          //  << blocktype << " " << blockfmt << " starting at " << std::dec << work.popped( ) << std::endl;
          switch ( blocktypefmt ) {
            case 0x0100:
              // sometimes our 68-byte block is only 66 bytes big! Luckily,
              // this only seems to happen when the blocktype is actuall 0x0D,
              // which we ignore anyway. It seems to be followed by 0x0100,
              // so just ignore this, too
              readDataBlock( info,{ } );
              break;
            case 0x024D:
              readDataBlock( info,{ SKIP6, AR1_M, AR1_S, AR1_D, SKIP2, AR1_R } );
              break;
            case 0x024E:
              readDataBlock( info,{ SKIP6, AR2_M, AR2_S, AR2_D, SKIP2, AR2_R } );
              break;
            case 0x024F:
              readDataBlock( info,{ SKIP6, AR3_M, AR3_S, AR3_D, SKIP2, AR3_R } );
              break;
            case 0x0250:
              readDataBlock( info,{ SKIP6, AR4_M, AR4_S, AR4_D, SKIP2, AR4_R } );
              break;
            case 0x034D:
              readDataBlock( info,{ SKIP6, PA1_M, PA1_S, PA1_D, SKIP2, PA1_R } );
              break;
            case 0x034E:
              readDataBlock( info,{ SKIP6, PA2_M, PA2_S, PA2_D, SKIP2, PA2_R } );
              break;
            case 0x034F:
              readDataBlock( info,{ SKIP6, PA3_M, PA3_S, PA3_D, SKIP2, PA3_R } );
              break;
            case 0x0350:
              readDataBlock( info,{ SKIP6, PA4_M, PA4_S, PA4_D, SKIP2, PA4_R } );
              break;
            case 0x044D:
              readDataBlock( info,{ SKIP6, LA1 } );
              break;
            case 0x054D:
              readDataBlock( info,{ SKIP6, CVP1 } );
              break;
            case 0x054E:
              readDataBlock( info,{ SKIP6, CVP2 } );
              break;
            case 0x054F:
              readDataBlock( info,{ SKIP6, CVP3 } );
              break;
            case 0x0550:
              readDataBlock( info,{ SKIP6, CVP4 } );
              break;
            case 0x064D:
              readDataBlock( info,{ SKIP6, ICP1, CPP1 } );
              break;
            case 0x064E:
              readDataBlock( info,{ SKIP6, ICP2, CPP2 } );
              break;
            case 0x064F:
              readDataBlock( info,{ SKIP6, ICP3, CPP3 } );
              break;
            case 0x0650:
              readDataBlock( info,{ SKIP6, ICP4, CPP4 } );
              break;
            case 0x074D:
              readDataBlock( info,{ SKIP6, SP1 } );
              break;
            case 0x0822:
              readDataBlock( info,{ SKIP6, RESP, APNEA } );
              break;
            case 0x0922:
              readDataBlock( info,{ SKIP6, BT, IT } );
              break;
            case 0x0A18:
              readDataBlock( info,{ SKIP6, NBP_M, NBP_S, NBP_D, SKIP2, CUFF } );
              break;
            case 0x0B2D:
              readDataBlock( info,{ SKIP6, SPO2_P, SPO2_R } );
              break;
            case 0x0C22:
            case 0x0C23:
              readDataBlock( info,{ SKIP6, TMP_1, TMP_2, DELTA_TMP } );
              break;
            case 0x0D56:
              readDataBlock( info,{ } );
              break;
            case 0x0D57:
              readDataBlock( info,{ SKIP6, SKIP, STV1 } );
              break;
            case 0x0D58:
            case 0x0D59:
              readDataBlock( info,{ } );
              break;
            case 0x0E36:
            case 0x0E4D:
              readDataBlock( info,{ SKIP6, CO2_EX, CO2_IN, CO2_RR, SKIP2, O2_EXP, O2_INSP } );
              break;
            case 0x104D:
              readDataBlock( info,{ SKIP6, UAC1_M, UAC1_S, UAC1_M, SKIP2, UAC1_R } );
              break;
            case 0x104E:
              readDataBlock( info,{ SKIP6, UAC2_M, UAC2_S, UAC2_M, SKIP2, UAC2_R } );
              break;
            case 0x104F:
              readDataBlock( info,{ SKIP6, UAC3_M, UAC3_S, UAC3_M, SKIP2, UAC3_R } );
              break;
            case 0x1050:
              readDataBlock( info,{ SKIP6, UAC4_M, UAC4_S, UAC4_M, SKIP2, UAC4_R } );
              break;
            case 0x14C2:
              readDataBlock( info,{ SKIP6, PT_RR, PEEP, MV, SKIP2, Fi02, TV, PIP, PPLAT, MAWP, SENS } );
              break;
            case 0x1D0A:
              readDataBlock( info,{ SKIP6, NBP_M, NBP_S, NBP_D, SKIP2, CUFF } );
              break;
            case 0x2ADB:
              readDataBlock( info,{ SKIP6, VENT, FLW_R, SKIP4, IN_HLD, SKIP2, PRS_SUP, INSP_TM, INSP_PC, I_E } );
              break;
            case 0x2ADC:
              readDataBlock( info,{ SKIP6, HF_FLW, HF_R, HF_PRS, SPONT_MV, SKIP2, SET_TV, SET_PCP, SET_IE, B_FLW, FLW_TRIG } );
              break;
            case 0x2A5C:
              readDataBlock( info,{ SKIP6, APRV_LO, APRV_HI, APRV_LO_T, SKIP2, APRV_HI_T, COMP, RESIS, MEAS_PEEP, INTR_PEEP, SPONT_R } );
              break;
            case 0x2A5D:
              readDataBlock( info,{ SKIP6, INSP_TV } );
              break;
            default:
              int type = ( blocktypefmt >> 8 );
              int fmt = ( blocktypefmt & 0xFF );
              unhandledBlockType( type, fmt );
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
      std::cerr << err.what( ) << " (chunk started at byte: "
          << chunkstart << ")" << std::endl;
      return ChunkReadResult::UNKNOWN_BLOCKTYPE;
    }
  }

  void StpGeReader::unhandledBlockType( unsigned int type, unsigned int fmt ) const {
    std::stringstream ss;
    ss << "unhandled block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
        << type << " " << fmt << " starting at " << std::dec << work.popped( );
    throw std::runtime_error( ss.str( ) );
  }

  dr_time StpGeReader::popTime( ) {
    // time is in little-endian format
    auto shorts = work.popvec( 4 );
    time_t time = ( ( shorts[1] << 24 ) | ( shorts[0] << 16 ) | ( shorts[3] << 8 ) | shorts[2] );
    return time * 1000;
  }

  StpGeReader::ChunkReadResult StpGeReader::readWavesBlock( SignalSet * info, const size_t& maxread ) {
    if ( 0x04 == work.read( ) ) {
      work.skip( ); // skip the 0x04
      auto oldseq = wavetracker.currentseq( );
      WaveSequenceResult wavecheck = wavetracker.newseq( popUInt8( ), currentTime );
      if ( WaveSequenceResult::TIMEBREAK == wavecheck ) {
        while ( wavetracker.writable( ) ) {
          output( ) << "time break at byte " << ( work.popped( ) - 1 ) << std::endl;
          wavetracker.flushone( info );
        }
      }
      else if ( WaveSequenceResult::DUPLICATE == wavecheck || WaveSequenceResult::SEQBREAK == wavecheck ) {
        output( ) << "wave sequence check: " << wavecheck << " (old/new): " << oldseq << "/"
            << wavetracker.currentseq( ) << " at byte " << ( work.popped( ) - 1 ) << std::endl;
      }
      // skip the other two bytes (don't know what they mean, if anything)
      work.skip( 2 );
    }

    //size_t wavestart = work.popped( );
    //output( ) << "waves section starts at: " << std::dec << wavestart << std::endl;

    // FIXME: I think we should read all values until the next chunk start
    // so we don't have to account for mysteriously appearing and disappearing
    // wave signals!

    static const unsigned int READCOUNTS[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

    int fa0dloop = 0;
    // we know we have at least one full segment in our work buffer,
    // so keep reading until we hit the next segment...then write the appropriate number of values to the signalset
    // and keep any overrun for the next loop
    while ( work.poppedSinceMark( ) < maxread ) {
      const unsigned int waveid = popUInt8( );
      const unsigned int countbyte = popUInt8( );

      // usually, we get 0x0B, but sometimes we get 0x3B. I don't know what
      // that means, but the B part seems to be the only thing that matters
      // so zero out the most significant bits
      unsigned int shifty = ( countbyte & 0b00000111 );
      if ( 0 == shifty || shifty > 7 ) {
        throw std::runtime_error( "Inconsistent wave information...file is corrupt?" );
      }
      unsigned int valstoread = READCOUNTS[shifty - 1];
      //      output( ) << "wave "
      //          << std::setfill( '0' ) << std::setw( 2 ) << std::hex << waveid << "; count:"
      //          << std::setfill( '0' ) << std::setw( 2 ) << std::hex << countbyte
      //          << "; shift code is: "
      //          << std::dec << shifty
      //          << "..." << std::dec << valstoread << " vals to read" << std::endl;

      if ( 0xFA == waveid && 0x0D == countbyte ) {
        fa0dloop++;
        //output( ) << "FA 0D section at byte: " << std::dec << ( work.popped( ) - 2 ) << " (loop " << fa0dloop << ")" << std::endl;
        // skip through this section to get to the next waveform section

        work.skip( isunity( ) ? 49 : 33 );
        //work.rewind( 33 );
        //        auto vec = work.popvec( 33 );
        //        for ( auto& i : vec ) {
        //          output( ) << "  " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << (unsigned int) i;
        //        }
        //        output( ) << std::endl;


        if ( 0x04 == work.read( ) ) {
          work.skip( ); // skip the 0x04
          auto oldseq = wavetracker.currentseq( );
          WaveSequenceResult wavecheck = wavetracker.newseq( popUInt8( ), currentTime );
          if ( WaveSequenceResult::TIMEBREAK == wavecheck ) {
            while ( wavetracker.writable( ) ) {
              output( ) << "time break (2) at byte " << ( work.popped( ) - 1 ) << std::endl;
              wavetracker.flushone( info );
            }
          }
          else if ( WaveSequenceResult::DUPLICATE == wavecheck || WaveSequenceResult::SEQBREAK == wavecheck ) {
            output( ) << "wave sequence check (2): " << wavecheck << " (old/new): " << oldseq << "/"
                << wavetracker.currentseq( ) << " at byte " << ( work.popped( ) - 1 ) << std::endl;
          }

          // skip the other two bytes (don't know what they mean, if anything)
          work.skip( 2 );
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
      else if ( 0 == StpGeReader::WAVELABELS.count( waveid ) ) {
        std::stringstream ss;
        ss << "unknown wave id/count: "
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << waveid << " "
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << countbyte
            << " starting at " << std::dec << work.popped( );
        std::string ex = ss.str( );
        throw std::runtime_error( ex );
      }
      else {
        //        if ( 23 == waveid ) {
        //          output( ) << "wave " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << waveid
        //              << " reading " << valstoread << " values starting at " << std::dec << work.popped( ) << std::endl;
        //        }
        //if ( countbyte > 0x0C ) {
        // we had a 0x3B or something
        //          output( ) << "countbyte discrepancy! "
        //              << std::setfill( '0' ) << std::setw( 2 ) << std::hex << countbyte
        //              << " at byte " << std::dec << ( work.popped( ) - 1 ) << std::endl;
        //}

        std::vector<int> vals;
        for ( size_t i = 0; i < valstoread; i++ ) {
          vals.push_back( popInt16( ) );
        }
        wavetracker.newvalues( waveid, vals );
      }
    }

    if ( 8 != fa0dloop ) {
      output( ) << "got " << fa0dloop << " loops instead of 8 at pos: " << work.popped( ) << std::endl;
    }

    while ( wavetracker.writable( ) ) {
      wavetracker.flushone( info );
    }

    return ChunkReadResult::OK;
  }

  void StpGeReader::readDataBlock( SignalSet * info, const std::vector<BlockConfig>& vitals, size_t blocksize ) {
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
            val = popInt8( );
            okval = ( val > -128 ); // compare signed ints
          }
          else {
            val = popInt16( );
            okval = ( val > -32767 );
          }

          if ( okval ) {
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
                //unsigned long pos = work.popped( ) - 1;
                //output( ) << "found 0xC9 at byte: " << std::dec << pos << std::endl;
                // got a C9, so check the previous 13 bytes for 0s
                bool found = true;
                for ( int i = 1; i < 14 && found; i++ ) {
                  //                  output( ) << "\tbyte " << ( pos - i ) << " is: "
                  //                          << std::setfill( '0' ) << std::setw( 2 ) << std::hex
                  //                          << (unsigned short) work.read( -i - 1 ) << std::endl;
                  if ( 0 != work.read( -i - 1 ) ) {
                    found = false;
                  }
                }
                if ( found ) {
                  ok = true;
                  if ( nullptr != size ) {
                    *size = ( work.poppedSinceMark( ) - 14 );
                  }
                  break;
                }
              }
            }
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

    if ( 28 == waveid ) {
      // if we have PA2-X, then this is a PA2 wave
      for ( auto& v : info->vitals( ) ) {
        if ( PA2_D.label == v->name( ) ) {
          return "PA2";
        }
      }
    }
    else if ( 29 == waveid ) {
      for ( auto& v : info->vitals( ) ) {
        if ( PA3_D.label == v->name( ) ) {
          return "PA3";
        }
      }
    }

    return name;
  }

  std::vector<StpReaderBase::StpMetadata> StpGeReader::parseMetadata( const std::string& input ) {
    std::vector<StpReaderBase::StpMetadata> metas;
    auto info = std::unique_ptr<SignalSet>{ std::make_unique<BasicSignalSet>( ) };
    StpGeReader reader;
    reader.setNonbreaking( true );
    int failed = reader.prepare( input, info.get( ) );
    if ( failed ) {
      std::cerr << "error while opening input file. error code: " << failed << std::endl;
      return metas;
    }
    reader.setMetadataOnly( );

    ReadResult last = ReadResult::FIRST_READ;

    bool okToContinue = true;
    while ( okToContinue ) {
      last = reader.fill( info.get(), last );
      switch ( last ) {
        case ReadResult::FIRST_READ:
          // NOTE: no break here
        case ReadResult::NORMAL:
          break;
        case ReadResult::END_OF_DAY:
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
          std::cerr << "error while reading input file" << std::endl;
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