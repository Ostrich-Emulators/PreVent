/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "BasicSignalSet.h"
#include "BasicSignalData.h"
#include "SignalUtils.h"

#include <limits>
#include <iostream>
#include "config.h"
#include "Log.h"

namespace FormatConverter{

  BasicSignalSet::BasicSignalSet( ) {
    setMeta( SignalData::TIMEZONE, "GMT" );
    setMeta( SignalData::BUILD_NUM, GIT_BUILD );
  }

  BasicSignalSet::BasicSignalSet( const BasicSignalSet& ) {
    setMeta( SignalData::TIMEZONE, "GMT" );
    setMeta( SignalData::BUILD_NUM, GIT_BUILD );
  }

  BasicSignalSet::~BasicSignalSet( ) { }

  BasicSignalSet BasicSignalSet::operator=(const BasicSignalSet&) {
    return *this;
  }

  std::vector<SignalData *> BasicSignalSet::vitals( ) const {
    std::vector<SignalData *> vec;
    for ( const auto& signal : vits ) {
      vec.push_back( signal.get( ) );
    }
    return vec;
  }

  std::vector<SignalData *> BasicSignalSet::waves( ) const {
    std::vector<SignalData *> vec;
    vec.reserve( wavs.size( ) );
    for ( const auto& signal : wavs ) {
      vec.push_back( signal.get( ) );
    }
    return vec;
  }

  dr_time BasicSignalSet::earliest( const TimeCounter& type ) const {
    dr_time early = std::numeric_limits<dr_time>::max( );

    if ( TimeCounter::VITAL == type || TimeCounter::EITHER == type ) {
      early = SignalUtils::firstlast( vitals( ) );
    }
    if ( TimeCounter::WAVE == type || TimeCounter::EITHER == type ) {
      dr_time w = SignalUtils::firstlast( waves( ) );
      if ( w < early ) {
        early = w;
      }
    }

    return early;
  }

  dr_time BasicSignalSet::latest( const TimeCounter& type ) const {
    dr_time last = 0;

    if ( TimeCounter::VITAL == type || TimeCounter::EITHER == type ) {
      SignalUtils::firstlast( vitals( ), nullptr, &last );
    }

    if ( TimeCounter::WAVE == type || TimeCounter::EITHER == type ) {
      dr_time w;
      SignalUtils::firstlast( waves( ), nullptr, &w );
      if ( w > last ) {
        last = w;
      }
    }

    return last;
  }

  std::unique_ptr<SignalData> BasicSignalSet::_createSignalData( const std::string& name,
      bool iswave, void * extra ) {
    return std::unique_ptr<SignalData>{ std::make_unique<BasicSignalData>( name, iswave ) };
  }

  SignalData * BasicSignalSet::addVital( const std::string& name, bool * added ) {
    for ( auto& x : vits ) {
      if ( x->name( ) == name ) {
        if ( nullptr != added ) {
          *added = false;
        }
        return x.get( );
      }
    }

    vits.push_back( _createSignalData( name, false ) );

    if ( nullptr != added ) {
      *added = true;
    }

    Log::debug( ) << "added vital: " << name << std::endl;

    return vits[vits.size( ) - 1].get( );
  }

  SignalData * BasicSignalSet::addWave( const std::string& name, bool * added ) {
    for ( auto& x : wavs ) {
      if ( x->name( ) == name ) {
        if ( nullptr != added ) {
          *added = false;
        }
        return x.get( );
      }
    }

    wavs.push_back( _createSignalData( name, true ) );

    if ( nullptr != added ) {
      *added = true;
    }

    Log::debug( ) << "added wave: " << name << std::endl;

    return wavs[wavs.size( ) - 1].get( );
  }

  void BasicSignalSet::reset( bool signalDataOnly ) {
    Log::debug( ) << "resetting signal set" << std::endl;

    vits.clear( );
    wavs.clear( );
    aux.clear( );
    if ( !signalDataOnly ) {
      metamap.clear( );
      metamap[SignalData::TIMEZONE] = "GMT";
      metamap[SignalData::BUILD_NUM] = GIT_BUILD;

      segs.clear( );
    }
  }

  std::vector<SignalData *> BasicSignalSet::allsignals( ) const {
    std::vector<SignalData *> vec;

    for ( const auto& m : vitals( ) ) {
      vec.push_back( m );
    }
    for ( const auto& m : waves( ) ) {
      vec.push_back( m );
    }

    return vec;
  }

  void BasicSignalSet::setMetadataFrom( const SignalSet& src ) {
    if ( this != &src ) {
      metamap.clear( );
      metamap.insert( src.metadata( ).begin( ), src.metadata( ).end( ) );

      segs.clear( );
      segs.insert( src.offsets( ).begin( ), src.offsets( ).end( ) );
    }
  }

  const std::map<std::string, std::string>& BasicSignalSet::metadata( ) const {
    return metamap;
  }

  void BasicSignalSet::setMeta( const std::string& key, const std::string & val ) {
    if ( val.empty( ) ) {
      metamap.erase( key );
    }
    else {
      metamap[key] = val;
    }
  }

  void BasicSignalSet::clearMetas( ) {
    metamap.clear( );
  }

  const std::map<long, dr_time>& BasicSignalSet::offsets( ) const {
    return segs;
  }

  void BasicSignalSet::addOffset( long seg, dr_time time ) {
    segs[seg] = time;
  }

  void BasicSignalSet::clearOffsets( ) {
    segs.clear( );
  }

  void BasicSignalSet::complete( ) {
    // nothing to do

    // for each vital/wave, write the min/max values as attributes
    Log::debug( ) << "completing signal set" << std::endl;

    for ( auto& data : vitals( ) ) {
      data->setMeta( "Min Value", data->lowwater( ) );
      data->setMeta( "Max Value", data->highwater( ) );
      data->setMeta( "Note on Min/Max", "Min and Max are raw values (not scaled)" );
    }
  }

  void BasicSignalSet::addAuxillaryData( const std::string& name, const TimedData& data ) {
    aux[name].push_back( data );
  }

  std::map<std::string, std::vector<TimedData>> BasicSignalSet::auxdata( ) {
    return aux;
  }
}