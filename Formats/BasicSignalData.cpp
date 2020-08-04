/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataSetDataCache.cpp
 * Author: ryan
 * 
 * Created on August 3, 2016, 7:47 AM
 */

#include "BasicSignalData.h"
#include "DataRow.h"
#include "Options.h"
#include "SignalUtils.h"
#include "TimeRange.h"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <limits>
#include <queue>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <charconv>
#include "config.h"
#include "Log.h"

namespace FormatConverter {

  const int BasicSignalData::DEFAULT_CACHE_LIMIT = 30000;

  BasicSignalData::BasicSignalData( const std::string& name, bool wavedata )
      : label( name ), firstdata( std::numeric_limits<dr_time>::max( ) ), lastdata( 0 ),
      datacount( 0 ), livecount( 0 ), popping( false ),
      iswave( wavedata ), highval( -std::numeric_limits<double>::max( ) ),
      lowval( std::numeric_limits<double>::max( ) ),
      nocache( FormatConverter::Options::asBool( FormatConverter::OptionsKey::NOCACHE ) ) {
    scale( 0 );
    setChunkIntervalAndSampleRate( 7, 1 ); // 7 is just an easy value to troubleshoot (it's not 2000 or 1024)
    setUom( "Uncalib" );

    setMeta( SignalData::MSM, SignalData::MISSING_VALUE );
    setMeta( SignalData::TIMEZONE, "UTC" );
    setMeta( "Note on Scale", "To get from a scaled value back to the real value, divide by 10^<scale>" );
  }

  BasicSignalData::~BasicSignalData( ) {
    data.clear( );
  }

  std::unique_ptr<SignalData> BasicSignalData::shallowcopy( bool includedates ) {
    auto copy = std::make_unique<BasicSignalData>( label );

    if ( includedates ) {
      copy->firstdata = this->firstdata;
      copy->lastdata = this->lastdata;
      copy->nocache = this->nocache;
    }

    for ( auto x : metad( ) ) {
      copy->setMeta( x.first, x.second );
    }
    for ( auto x : metai( ) ) {
      copy->setMeta( x.first, x.second );
    }
    for ( auto x : metas( ) ) {
      copy->setMeta( x.first, x.second );
    }
    copy->popping = this->popping;
    for ( std::string& s : extras( ) ) {
      copy->extras( ).push_back( s );
    }
    return std::move( copy );
  }

  dr_time BasicSignalData::startTime( ) const {
    return firstdata;
  }

  dr_time BasicSignalData::endTime( ) const {
    return lastdata;
  }

  std::unique_ptr<FormatConverter::DataRow> BasicSignalData::pop( ) {
    if ( !popping ) {
      startPopping( );
    }

    if ( cachefile && data.empty( ) ) {
      int lines = uncache( );
      livecount += lines;
      if ( 0 == lines ) {
        cachefile.reset( );
      }
    }

    datacount--;
    livecount--;
    std::unique_ptr<DataRow> row = std::move( data.front( ) );
    data.pop_front( );
    return row;
  }

  size_t BasicSignalData::size( ) const {
    return datacount;
  }

  const std::string& BasicSignalData::name( ) const {
    return label;
  }

  bool BasicSignalData::startPopping( ) {
    popping = true;
    if ( cachefile ) {
      if ( !cache( ) ) { // copy any extra rows to disk
        return false;
      }
      std::rewind( cachefile->file );
    }
    return true;
  }

  double BasicSignalData::highwater( ) const {
    return highval;
  }

  double BasicSignalData::lowwater( ) const {
    return lowval;
  }

  bool BasicSignalData::cache( ) {
    if ( nocache ) {
      return true;
    }

    if ( !cachefile ) {
      cachefile = SignalUtils::tmpf( );
    }

    const auto SIZEOFTIME = sizeof ( dr_time );
    const auto SIZEOFINT = sizeof ( int );

    while ( !data.empty( ) ) {
      std::stringstream ss;
      std::string extradata;
      int ok = 0;
      std::unique_ptr<DataRow> a = std::move( data.front( ) );
      data.pop_front( );
      const auto DATACOUNT = static_cast<int> ( a->ints( ).size( ) );

      if ( !a->extras.empty( ) ) {
        for ( const auto& x : a->extras ) {
          ss << "|" << x.first << "=" << x.second;
        }
        ss << '\n';
        extradata.assign( ss.str( ) );
      }

      // the offset is the where the next record starts, which doesn't include
      // this row's time (because it's already written) or the size itself
      // (because it will already have been read). We're left with the
      // ints for scale and each datapoint, plus an int for data point count
      int nextrecord_offset =
          DATACOUNT * SIZEOFINT // values array
          + SIZEOFINT // scale
          + SIZEOFINT // data count
          ;
      nextrecord_offset += extradata.size( ); // extradata

      ok += std::fwrite( &a->time, SIZEOFTIME, 1, cachefile->file );
      ok += std::fwrite( &nextrecord_offset, SIZEOFINT, 1, cachefile->file );
      ok += std::fwrite( &a->scale, SIZEOFINT, 1, cachefile->file );
      ok += std::fwrite( &DATACOUNT, SIZEOFINT, 1, cachefile->file );
      ok += std::fwrite( a->data.data( ), SIZEOFINT, DATACOUNT, cachefile->file );

      if ( !extradata.empty( ) ) {
        std::fputs( extradata.c_str( ), cachefile->file );
      }

      if ( ok != DATACOUNT + 4 ) {
        return false;
      }
    }

    std::fflush( cachefile->file ); // for windows
    livecount = 0;
    return true;
  }

  size_t BasicSignalData::inmemsize( ) const {
    return data.size( );
  }

  bool BasicSignalData::add( std::unique_ptr<DataRow> row ) {

    if ( livecount >= DEFAULT_CACHE_LIMIT ) {
      // copy current data list to disk
      if ( !cache( ) ) {
        return false;
      }
    }

    datacount++;
    livecount++;

    int rowscale = row->scale;

    double pow10 = std::pow( 10, rowscale );
    int hiv = std::numeric_limits<int>::min( );
    int lov = std::numeric_limits<int>::max( );
    bool check_scale = false;
    for ( auto& v : row->data ) {
      if ( v != SignalData::MISSING_VALUE ) {
        hiv = std::max( hiv, v );
        lov = std::min( lov, v );
        check_scale = true;
      }
    }

    if ( check_scale ) {
      double rowlo = static_cast<double> ( lov ) / pow10;
      double rowhi = static_cast<double> ( hiv ) / pow10;

      lowval = std::min( rowlo, lowval );
      highval = std::max( rowhi, highval );

      if ( rowscale > scale( ) ) {
        scale( rowscale );
      }
    }

    if ( !row->extras.empty( ) ) {
      for ( const auto& x : row->extras ) {
        extras( x.first );
      }
    }

    lastdata = std::max( row->time, lastdata );
    firstdata = std::min( row->time, firstdata );
    data.push_back( std::move( row ) );
    return true;
  }

  void BasicSignalData::setWave( bool wave ) {
    iswave = wave;
  }

  bool BasicSignalData::wave( ) const {
    return iswave;
  }

  int BasicSignalData::uncache( int max ) {
    int loop = 0;
    const int BUFFSZ = 1024 * 16;
    char buff[BUFFSZ];

    dr_time t;
    int scale;
    int counter;
    int offset;
    int ok = 0;
    std::map<std::string, std::string> attrs;

    const auto SIZEOFTIME = sizeof ( dr_time );
    const auto SIZEOFINT = sizeof ( int );

    while ( loop < max ) {
      attrs.clear( );

      ok += std::fread( &t, SIZEOFTIME, 1, cachefile->file );
      ok += std::fread( &offset, SIZEOFINT, 1, cachefile->file );
      auto offset_start = std::ftell( cachefile->file );
      ok += std::fread( &scale, SIZEOFINT, 1, cachefile->file );
      ok += std::fread( &counter, SIZEOFINT, 1, cachefile->file );

      auto vals = std::vector<int>( counter );
      ok += std::fread( &vals[0], SIZEOFINT, counter, cachefile->file );

      if ( ok < counter + 4 ) {
        std::cerr << "error reading cache file for signal: " << label << std::endl;
      }

      if ( static_cast<int> ( std::ftell( cachefile->file ) - offset_start ) < offset ) {

        char * justread = std::fgets( buff, BUFFSZ, cachefile->file );
        if ( nullptr != justread ) {
          auto extras = std::string_view( justread );

          // split on |, then split again on "="
          auto pieces = SignalUtils::splitcsv( extras, '|' );
          for ( const auto& keyandval : pieces ) {
            auto kv = SignalUtils::splitcsv( keyandval, '=' );
            if ( 2 == kv.size( ) ) {
              auto newlinepos = kv[1].rfind( '\n' );
              if ( std::string::npos != newlinepos ) {
                kv[1].remove_suffix( 1 );
              }
              attrs[std::string( kv[0] )] = std::string( kv[1] );
            }
          }
        }
      }

      data.push_back( std::make_unique<DataRow>( t, vals, scale, attrs ) );
      loop++;
    }

    return loop;
  }

  std::unique_ptr<TimeRange> BasicSignalData::times( ) {
    auto range = std::make_unique<TimeRange>( );

    const auto SLABSIZE = 1024 * 64;
    auto dates = std::vector<dr_time>{ };
    dates.reserve( SLABSIZE );

    const auto SIZET = sizeof (dr_time );
    const auto SIZEI = sizeof (int );

    if ( cachefile ) {
      // read the dates out of the file first
      std::rewind( cachefile->file );
      dr_time timer;
      size_t offset = 0;

      while ( std::fread( &timer, SIZET, 1, cachefile->file ) > 0 ) {
        auto ok = std::fread( &offset, SIZEI, 1, cachefile->file );
        dates.push_back( timer );

        // push our date slab to the file
        if ( dates.size( ) > SLABSIZE ) {
          range->push_back( dates );
          dates.clear( );
        }

        auto ok2 = std::fseek( cachefile->file, offset, SEEK_CUR );

        if ( ok < 1 || ok2 != 0 ) {
          Log::debug() << "blamo!" << std::endl;
        }
      }
      // when we get here, our file pointer has traveled back to the end of the file
    }

    // now add the dates currently in our cache
    for ( auto it = data.begin( ); it != data.end( ); it++ ) {
      dates.push_back( ( *it )->time );
    }
    range->push_back( dates );
    return range;
  }

  void BasicSignalData::setMetadataFrom( const SignalData& model ) {
    metadatai.clear( );
    metadatai.insert( model.metai( ).begin( ), model.metai( ).end( ) );

    metadatas.clear( );
    metadatas.insert( model.metas( ).begin( ), model.metas( ).end( ) );

    metadatad.clear( );
    metadatad.insert( model.metad( ).begin( ), model.metad( ).end( ) );

    extrafields.clear( );
    std::vector<std::string> vec = model.extras( );
    extrafields.insert( vec.begin( ), vec.end( ) );
  }

  std::vector<std::string> BasicSignalData::extras( ) const {
    return std::vector<std::string>( extrafields.begin( ), extrafields.end( ) );
  }

  void BasicSignalData::extras( const std::string& ext ) {
    extrafields.insert( ext );
  }

  void BasicSignalData::setMeta( const std::string& key, const std::string& val ) {
    metadatas[key] = val;
  }

  void BasicSignalData::setMeta( const std::string& key, int val ) {
    metadatai[key] = val;
  }

  void BasicSignalData::setMeta( const std::string& key, double val ) {
    metadatad[key] = val;
  }

  void BasicSignalData::erases( const std::string& key ) {
    if ( "" == key ) {
      metadatas.clear( );
    }
    else {
      metadatas.erase( key );
    }
  }

  void BasicSignalData::erasei( const std::string& key ) {
    if ( "" == key ) {
      metadatai.clear( );
    }
    else {
      metadatai.erase( key );
    }
  }

  void BasicSignalData::erased( const std::string& key ) {
    if ( "" == key ) {
      metadatad.clear( );
    }
    else {
      metadatad.erase( key );
    }
  }

  const std::map<std::string, std::string>& BasicSignalData::metas( ) const {
    return metadatas;
  }

  const std::map<std::string, int>& BasicSignalData::metai( ) const {
    return metadatai;
  }

  const std::map<std::string, double>& BasicSignalData::metad( ) const {
    return metadatad;
  }

  void BasicSignalData::recordEvent( const std::string& eventtype, const dr_time& time ) {
    if ( 0 == namedevents.count( eventtype ) ) {
      namedevents[eventtype] = std::vector<dr_time>( );
      namedevents[eventtype].push_back( time );
    }
    else {
      auto& list = namedevents[eventtype];
      if ( list[list.size( ) - 1] != time ) {
        list.push_back( time );
      }
    }
  }

  std::vector<std::string> BasicSignalData::eventtypes( ) {
    std::vector<std::string> keys;
    for ( auto& x : namedevents ) {
      keys.push_back( x.first );
    }
    return keys;
  }

  std::vector<dr_time> BasicSignalData::events( const std::string& type ) {
    return ( 0 == namedevents.count( type )
        ? std::vector<dr_time>( )
        : namedevents.at( type ) );
  }

  void BasicSignalData::addAuxillaryData( const std::string& name, const TimedData& data ) {
    aux[name].push_back( data );
  }

  std::map<std::string, std::vector<TimedData>> BasicSignalData::auxdata( ) {
    return aux;
  }
}