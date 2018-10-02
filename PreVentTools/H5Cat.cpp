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

H5Cat::H5Cat( const std::string& outfile ) : output( outfile ), doduration( false ),
    havestart(false), doclip( false ), duration_ms( 0 ), start(0 ), end( 0 ) {
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

void H5Cat::cat( std::vector<std::string>& filesToCat ) {
  // we need to open all the files and see which datasets we have
  // it's a lot easier to store these things as SignalData objects
  // even though that will write extra temp files
  Hdf5Reader rdr;
  std::unique_ptr<SignalSet> alldata( new BasicSignalSet( ) );

  std::cout << "duration not accounted for yet" << std::endl;
  std::cout << "duration? " << doduration << " time: " << duration_ms << " from " << start << std::endl;


  if ( doduration && !havestart ) {
    // look at the first file to get the start time
    std::unique_ptr<SignalSet> junk( new BasicSignalSet( ) );
    rdr.prepare( filesToCat.at( 0 ), junk );
    rdr.fill( junk );

    start = junk->earliest( );
    havestart = true;
    alldata.reset( new ClippingSignalSet( alldata.release( ), &start, &start + duration_ms ) );
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
  fn.outputdir( "." );
  wrt.filenamer( fn );




  wrt.write( nullrdr, alldata );
}

void H5Cat::cat( const std::string& outfile,
    std::vector<std::string>& filesToCat ) {
  H5Cat( outfile ).cat( filesToCat );
}
