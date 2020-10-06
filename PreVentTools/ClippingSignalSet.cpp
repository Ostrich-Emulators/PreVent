/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "ClippingSignalSet.h"
#include "SignalDataWrapper.h"
#include "BasicSignalSet.h"
namespace FormatConverter{

  class ClippingSignalData : public SignalDataWrapper{
  public:

    ClippingSignalData( ClippingSignalSet& set, SignalData * signal )
        : SignalDataWrapper( signal ), parent( set ) { }

    bool add( std::unique_ptr<DataRow> row ) {
      if ( parent.timeok( row->time ) ) {
        return SignalDataWrapper::add( std::move( row ) );
      }
      return true;
    }

  private:
    ClippingSignalSet & parent;
  };

  ClippingSignalSet::ClippingSignalSet( dr_time * starttime, dr_time * endtime ) : SignalSetWrapper( new BasicSignalSet( ) ) {
    init( starttime, endtime );
  }

  ClippingSignalSet::ClippingSignalSet( std::unique_ptr<SignalSet> w, dr_time * starttime, dr_time * endtime )
      : SignalSetWrapper( w ) {
    init( starttime, endtime );
  }

  ClippingSignalSet::ClippingSignalSet( SignalSet * w, dr_time * starttime, dr_time * endtime )
      : SignalSetWrapper( w ) {
    init( starttime, endtime );
  }

  std::unique_ptr<ClippingSignalSet> ClippingSignalSet::duration( const dr_time& for_ms, dr_time * starttime ) {
    auto set = std::make_unique<ClippingSignalSet>( starttime );
    set->initForDuration( for_ms );
    return set;
  }

  ClippingSignalSet::~ClippingSignalSet( ) { }

  void ClippingSignalSet::init( dr_time* starttime, dr_time* endtime ) {
    havefirsttime = false;

    checkstart = ( nullptr != starttime );
    if ( checkstart ) {
      start = *starttime;
      havefirsttime = true;
    }

    checkend = ( nullptr != endtime );
    if ( checkend ) {
      end = *endtime;
    }
  }

  void ClippingSignalSet::initForDuration( const dr_time& dur_ms ) {
    checkduration = true;
    duration_ms = dur_ms;
  }

  std::unique_ptr<SignalData> ClippingSignalSet::_createSignalData( const std::string& name,
      bool iswave, void * extra ) {
    auto ret = SignalSetWrapper::_createSignalData( name, iswave, extra );
    ret.reset( new ClippingSignalData( *this, ret.release() ) );
    return ret;
  }

  bool ClippingSignalSet::timeok( const dr_time& time ) {
    if ( checkduration && ( time < start || !havefirsttime ) ) {
      start = time;
      end = start + duration_ms;
      havefirsttime = true;
    }

    bool ok = true;
    if ( checkstart && time < start ) {
      ok = false;
    }

    if ( ok && checkend && time >= end ) {
      ok = false;
    }

    return ok;
  }
}