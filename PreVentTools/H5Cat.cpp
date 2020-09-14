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
#include "Log.h"

#include <algorithm>    // std::sort

namespace FormatConverter{

  H5Cat::H5Cat( const std::string& outfile ) : output( outfile ), doduration( false ),
      havestart( false ), doclip( false ), duration_ms( 0 ), start( 0 ), end( 0 ) { }

  H5Cat::H5Cat( const H5Cat& orig ) : output( orig.output ),
      doduration( orig.doduration ), havestart( orig.havestart ),
      doclip( orig.doclip ), duration_ms( orig.duration_ms ), start( orig.start ),
      end( orig.end ) { }

  H5Cat::~H5Cat( ) { }

  void H5Cat::setClipping( const dr_time& starttime, const dr_time& endtime ) {
    start = starttime;
    end = endtime;
    doclip = true;
  }

  bool H5Cat::filesorter( const std::string& a, const std::string& b ) {
    std::unique_ptr<Reader> areader( Reader::get( FormatConverter::Formats::guess( a ) ) );
    std::unique_ptr<Reader> breader( Reader::get( FormatConverter::Formats::guess( b ) ) );

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
    auto alldata = std::unique_ptr<SignalSet>{ std::make_unique<BasicSignalSet>( ) };

    Log::debug( ) << "files to cat:";
    for ( auto x : filesToCat ) {
      Log::debug( ) << " " << x;
    }
    Log::debug( ) << std::endl;

    if ( doclip ) {
      alldata.reset( new ClippingSignalSet( alldata.release( ), &start, &end ) );

      // if a file's start time is after our end time, or our end
      // time is before our start time remove that file from our list
      filesToCat.erase( std::remove_if( filesToCat.begin( ),
          filesToCat.end( ),
          [this]( std::string filename ) {
            auto reader = Reader::get( FormatConverter::Formats::guess( filename ) );

            std::map<std::string, std::string> map;
            reader->getAttributes( filename, map );

            const dr_time startt = ( 0 == map.count( SignalData::STARTTIME )
                ? std::numeric_limits<dr_time>::max( )
                : std::stol( map.at( SignalData::STARTTIME ) ) );
            const dr_time endt = ( 0 == map.count( SignalData::ENDTIME )
                ? std::numeric_limits<dr_time>::min( )
                : std::stol( map.at( SignalData::ENDTIME ) ) );
            //            std::cout << filename << std::endl
            //                << "\tglobal start/end:\t" << this->start << "\t" << this->end << std::endl
            //                << "\tstart/end:\t\t" << startt << "\t" << endt << std::endl
            //                << "\tstartt>=end:" << ( startt <= this->end ) << std::endl
            //                << "\tendt<=start:" << ( endt <= this->start ) << std::endl
            //                << "\treturning " << ( startt >= this->end || endt <= this->start ) << std::endl;

            auto removeit = ( startt >= this->end || endt <= this->start );
            if ( removeit ) {
              Log::debug( ) << "ignoring " << filename << " from list because it's clearly out of range" << std::endl;
            }
            return removeit;
          } ),
      filesToCat.end( ) );
    }

    for ( const auto& file : filesToCat ) {
      Log::debug( ) << "  " << file << std::endl;
      auto junk = BasicSignalSet{ };
      rdr.prepare( file, &junk );
      rdr.fill( alldata.get( ) );
      rdr.finish( );
    }

    alldata->setMeta( "Source Reader", rdr.name( ) );

    Hdf5Writer wrt;
    auto nullrdr = NullReader{ rdr.name( ) };
    FileNamer fn = FileNamer::parse( output );

    fn.tofmt( wrt.ext( ) );
    fn.inputfilename( "-" );
    wrt.filenamer( fn );

    wrt.write( &nullrdr, alldata.get( ) );
    Log::info( ) << "  written to " << this->output << std::endl;
  }

  void H5Cat::cat( const std::string& outfile, std::vector<std::string>& filesToCat ) {
    H5Cat( outfile ).cat( filesToCat );
  }
}