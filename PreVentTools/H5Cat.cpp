/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   H5Cat.cpp
 * Author: ryan
 * 
 * Created on February 28, 2018, 10:31 AM
 */

#include <H5Cpp.h>
#include <iostream>
#include <memory>

#include "H5Cat.h"
#include "Hdf5Reader.h"
#include "Hdf5Writer.h"
#include "BasicSignalData.h"
#include "BasicSignalSet.h"
#include "NullReader.h"
#include "FileNamer.h"
#include "ClippingSignalSet.h"

#include <algorithm>    // std::sort

H5Cat::H5Cat( const std::string& outfile ) : output( outfile ), doduration( false ),
havestart( false ), doclip( false ), duration_ms( 0 ), start( 0 ), end( 0 ) {
}

H5Cat::H5Cat( const H5Cat& orig ) : output( orig.output ), doduration( orig.doduration ), havestart( orig.havestart ),
doclip( orig.doclip ), start( orig.start ), end( orig.end ), duration_ms( orig.duration_ms ) {
}

H5Cat::~H5Cat( ) {
}

void H5Cat::setDuration( const dr_time& dur_ms, dr_time * start ) {
  havestart = ( nullptr != start );
  duration_ms = dur_ms;
  doduration = true;
}

void H5Cat::setClipping( const dr_time& starttime, const dr_time& endtime ) {
  start = starttime;
  end = endtime;
  doclip = true;
}

bool H5Cat::filesorter( const std::string& a, const std::string& b ) {
  std::unique_ptr<Reader> areader( Reader::get( Formats::guess( a ) ) );
  std::unique_ptr<Reader> breader( Reader::get( Formats::guess( b ) ) );

  std::map<std::string, std::string> amap;
  std::map<std::string, std::string> bmap;
  areader->getAttributes( a, amap );
  breader->getAttributes( b, bmap );

  dr_time astart = ( 0 == amap.count( SignalData::STARTTIME )
      ? 0
      : std::stol( amap.at( SignalData::STARTTIME ) ) );
  dr_time bstart = ( 0 == bmap.count( SignalData::STARTTIME )
      ? 0
      : std::stol( bmap.at( SignalData::STARTTIME ) ) );

  return ( astart < bstart );
}

void H5Cat::cat( std::vector<std::string>& filesToCat ) {
  // we need to open all the files and see which datasets we have
  // it's a lot easier to store these things as SignalData objects
  // even though that will write extra temp files
  Hdf5Reader rdr;
  std::unique_ptr<SignalSet> alldata( new BasicSignalSet( ) );

  std::cout << "duration? " << doduration << " time: " << duration_ms << " from " << start << std::endl;

  if ( doduration && !havestart ) {
    // order the files so we can figure out our start time
    std::sort( filesToCat.begin( ), filesToCat.end( ), filesorter );

    std::unique_ptr<Reader> areader( Reader::get( Formats::guess( filesToCat[0] ) ) );
    std::map<std::string, std::string> amap;
    areader->getAttributes( filesToCat[0], amap );
    start = ( 0 == amap.count( SignalData::STARTTIME )
        ? 0
        : std::stol( amap.at( SignalData::STARTTIME ) ) );
    havestart = true;
    end = start + duration_ms;
    doclip = true;
  }
  if ( doclip ) {
    alldata.reset( new ClippingSignalSet( alldata.release( ), &start, &end ) );
  }

  for ( const auto& file : filesToCat ) {
    std::cout << "  " << file << std::endl;
    std::unique_ptr<SignalSet> junk( new BasicSignalSet( ) );
    rdr.prepare( file, junk );
    rdr.fill( alldata );
    rdr.finish( );
  }

  alldata->setMeta( "Source Reader", rdr.name( ) );

  Hdf5Writer wrt;
  std::unique_ptr<Reader> nullrdr( new NullReader( rdr.name( ) ) );
  FileNamer fn = FileNamer::parse( output );

  fn.tofmt( wrt.ext( ) );
  fn.inputfilename( "-" );
  wrt.filenamer( fn );

  wrt.write( nullrdr, alldata );
}

void H5Cat::cat( const std::string& outfile,
    std::vector<std::string>& filesToCat ) {
  H5Cat( outfile ).cat( filesToCat );
}
