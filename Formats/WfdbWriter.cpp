/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WfdbWriter.h"
#include <cstdio>
#include <iostream>
#include <set>
#include <unistd.h>
#include <ctime>
#include <sstream>

#include "SignalSet.h"
#include "SignalData.h"
#include "SignalUtils.h"
#include "config.h"
#include "FileNamer.h"
namespace FormatConverter{

  WfdbWriter::WfdbWriter( ) : Writer( "hea" ) { }

  WfdbWriter::WfdbWriter( const WfdbWriter& ) : Writer( "hea" ) { }

  WfdbWriter::~WfdbWriter( ) { }

  int WfdbWriter::initDataSet( ) {
    currdir = getcwd( NULL, 0 );
    output( ) << "WFDB directory code has been changed, and MUST be refactored. " << std::endl;
    // FIXME: need to figure out where our output directory is
    //std::string directory = filenamer( ).outputdir( );
    std::string directory( "." );
    int x = chdir( directory.c_str( ) );
    if ( 0 != x ) {
      return -1;
    }

    files.clear( );

    fileloc = filenamer( ).filename( ).substr( directory.size( ) );
    std::string rectest = fileloc + "-20170101"; // we're going to replace the date
    int headerok = newheader( (char *) rectest.c_str( ) ); // check that our header file is valid
    if ( 0 == headerok ) {
      // remove the file
      std::string header = directory + rectest + ".hea";
      remove( header.c_str( ) );
    }
    return headerok;
  }

  std::vector<std::string> WfdbWriter::closeDataSet( ) {
    int x = chdir( currdir.c_str( ) );
    x += 0; // just to avoid compiler warning
    wfdbquit( );
    return files;
  }

  int WfdbWriter::drain( SignalSet * info ) {
    output( ) << "Error: WFDB writing not yet implemented" << std::endl;
    return -1;


    std::map<double, std::vector < SignalData *>> freqgroups;
    for ( auto ds : info->vitals( ) ) {
      double freq = ds->hz( );
      freqgroups[freq].push_back( ds );
    }

    for ( auto& ds : info->waves( ) ) {
      double freq = ds->hz( );
      freqgroups[freq].push_back( ds );
    }

    for ( auto& ds : freqgroups ) {
      write( ds.first, ds.second, filenamer( ).filenameNoExt( info ) );
    }

    return 0;
  }

  int WfdbWriter::write( double freq, std::vector<SignalData *> data,
      const std::string& namestart ) {
    setsampfreq( freq );

    sigmap.clear( );
    for ( auto& signal : data ) {
      const std::string& name = signal->name( );
      sigmap[name].units = (char *) signal->uom( ).c_str( );
      sigmap[name].group = 0;
      sigmap[name].desc = (char *) name.c_str( );
      sigmap[name].fmt = 16;
    }

    dr_time firstTime = SignalUtils::firstlast( data );
    std::string suffix = ( freq < 1 ? "vitals" : std::to_string( (int) freq ) + "hz" );
    std::string datafile = namestart + "_" + suffix + ".dat";
    std::string headerfile = namestart + "_" + suffix + ".hea";
    std::string cwd = getcwd( NULL, 0 );
    files.push_back( cwd + dirsep + datafile );
    files.push_back( cwd + dirsep + headerfile );

    WFDB_Siginfo sigs[sigmap.size( )];
    int i = 0;
    for ( const auto& signal : data ) {
      sigs[i] = sigmap[signal->name( )];
      sigs[i].fname = (char *) datafile.c_str( );
      i++;
    }

    if ( osigfopen( sigs, sigmap.size( ) ) < (int) sigmap.size( ) ) {
      return -1;
    }

    time_t mytime = firstTime / 1000;
    tm * t = gmtime( &mytime );
    if ( 0 != ( t->tm_hour + t->tm_min + t->tm_sec ) ) { // not 00:00:00 (midnight)?
      char timestr[sizeof "00:00:00"];
      std::strftime( timestr, sizeof timestr, "%T", t );
      setbasetime( timestr );
    }

    syncAndWrite( freq, data );

    newheader( (char *) headerfile.c_str( ) ); // we know this is a valid record name
    return 0;
  }

  void WfdbWriter::syncAndWrite( double freq, std::vector<SignalData *> olddata ) {
    std::vector<std::vector<int>> data = SignalUtils::syncDatas( olddata );

    // FIXME: refactor all this stuff!



    auto first = olddata[0];
    if ( first->wave( ) ) {
      //int ifrq = ( olddata.begin( ) )->get( )->readingsPerChunk( );

      // waveforms

      //      for ( std::vector<int> rowcols : data ) {
      //        int cols = rowcols.size( );
      //
      //        WFDB_Sample samples[cols][ifrq] = { 0 };
      //        for ( int col = 0; col < cols; col++ ) {
      //          std::vector<int> slices = FormatConverter::DataRow::ints( rowcols[col] );
      //          size_t numslices = slices.size( );
      //          for ( size_t slice = 0; slice < numslices; slice++ ) {
      //            samples[col][slice] = slices[slice];
      //          }
      //        }
      //
      //        // we have all the samples for one second of readings, so add them
      //        // to our datafile one row at a time.
      //        WFDB_Sample onerow[rowcols.size( )];
      //        for ( int f = 0; f < ifrq; f++ ) {
      //          for ( int c = 0; c < cols; c++ ) {
      //            onerow[c] = samples[c][f];
      //          }
      //          putvec( &onerow[0] );
      //        }
      //      }
      //    }
      //    else {
      //      // vital signs
      //      for ( std::vector<int> row : data ) {
      //        std::vector<WFDB_Sample> vec;
      //        for ( int rowcols : row ) {
      //          vec.push_back( rowcols );
      //        }
      //        putvec( &vec[0] );
      //      }
    }
  }
}