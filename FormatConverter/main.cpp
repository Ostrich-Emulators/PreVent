// XmlToHdf5.cpp : Defines the entry point for the console application.
//

#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <wfdb/wfdb.h>

#include "H5Cpp.h"

#include "CacheFileReader.h"
#include "FromReader.h"
#include "ToWriter.h"
#include "Formats.h"
#include "DataSetDataCache.h"
#include "DataRow.h"

#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif

using namespace std;

void helpAndExit( char * progname, std::string msg = "" ) {
  std::cerr << msg << std::endl
      << "Syntax: " << progname << " --from <format> --to <format> <file>..."
      << std::endl << "\t-f or --from <input format>"
      << std::endl << "\t-t or --to <output format>"
      << std::endl << "\tValid formats: wfdb, hdf5, stpxml"
      << std::endl << "\tIf file is -, stdin is read for input, and the format is assumed to be our zl format, regardless of --from option"
      << std::endl << std::endl;
  exit( 1 );
}

struct option longopts[] = {
  { "from", required_argument, NULL, 'f' },
  { "to", required_argument, NULL, 't' },
  { 0, 0, 0, 0 }
};

int main( int argc, char** argv ) {
  char c;
  extern int optind;
  extern char * optarg;
  std::string fromstr = "";
  std::string tostr = "";

  while ( ( c = getopt_long( argc, argv, ":f:t:", longopts, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'f':
        fromstr = optarg;
        break;
      case 't':
        tostr = optarg;
        break;
      case '?':
      default:
        helpAndExit( argv[0] );
        break;
    }
  }

  if ( ( optind + 1 ) > argc ) {
    helpAndExit( argv[0], "no file specified" );
  }

  if ( "-" == argv[optind] ) {
    fromstr = "zl";
  }

  if ( "" == fromstr ) {
    helpAndExit( argv[0], "--from not specified" );
  }
  else if ( "" == tostr ) {
    helpAndExit( argv[0], "--to not specified" );
  }

  // get a reader
  Format fromfmt = Formats::getValue( fromstr );
  Format tofmt = Formats::getValue( tostr );
  if ( Format::UNRECOGNIZED == fromfmt ) {
    helpAndExit( argv[0], "--from format not recognized" );
  }
  if ( Format::UNRECOGNIZED == tofmt ) {
    helpAndExit( argv[0], "--to format not recognized" );
  }


  std::unique_ptr<FromReader> from = FromReader::get( fromfmt );
  std::unique_ptr<ToWriter> to = ToWriter::get( tofmt );

  // send the files through
  for ( int i = optind; i < argc; i++ ) {
    std::cout << "converting " << argv[i]
        << " from " << fromstr
        << " to " << tostr << std::endl;
    from->reset( argv[i] );

    const auto& vs = from->vitals( );
    for ( const auto& mapit : vs ) {
      std::cout << mapit.first << std::endl;
      mapit.second->startPopping( );

      int rows = mapit.second->size( );
      for ( int i = 0; i < rows; i++ ) {
        std::unique_ptr<DataRow> row = std::move( mapit.second->pop( ) );
        std::cout << "\t" << row->time << " " << row->data << " "
            << mapit.second->uom( ) << std::endl;
      }
    }
  }

  exit( 0 );




  int i;
  WFDB_Sample v[500];
  WFDB_Siginfo s[500];

  const char * recname = "./Site01_0204_vitals";
  if ( isigopen( (char *) recname, s, 500 ) < 1 ) {
    exit( 1 );
  }
  for ( i = 0; i < 10; i++ ) {
    if ( getvec( v ) < 0 )
      break;
    printf( "%d\t%d\n", v[0], v[1] );
  }

  wfdbquit( );



  if ( argc < 5 ) {
    helpAndExit( argv[0] );
  }

  std::string xmlpath( argv[1] );
  std::string hdf5path( argv[2] );

  bool rollover = true;
  int compression = 6;
  std::string prefix = "";
  std::string usecache( "auto" );

  for ( int i = 3; i < argc; i++ ) {
    std::string arg( argv[i] );
    if ( "--no-rollover" == arg ) {
      rollover = false;
    }
    else {
      if ( argc <= i + 1 ) {
        helpAndExit( argv[0], "missing required argument for " + arg );
      }
      std::string argval( argv[++i] );

      if ( "--compression" == arg ) {
        compression = std::stoi( argval );
      }
      else if ( "--use-caches" == arg ) {
        usecache = argval;
      }
      else if ( "--prefix" == arg ) {
        prefix = argval;
      }
      else {
        helpAndExit( argv[0], "Unknown option: " + arg );
      }
    }
  }

  struct stat xmlstat;
  if ( "-" == xmlpath || "-zl" == xmlpath ) {
    xmlstat.st_size = 0;
    usecache = ( rollover ? "false" : "true" );
  }
  else {
#ifdef __linux__
    if ( stat( xmlpath.c_str( ), &xmlstat ) < 0 ) {
      perror( xmlpath.c_str( ) );
      exit( EXIT_FAILURE );
    }
#else
    // can't get stat to work with windows's \\ separator
    xmlstat.st_size = 0;
    usecache = ( rollover ? "false" : "true" );
#endif
  }

  // if we're given a big file, and we're not rolling over on days, then
  // worry about using temp files instead of memory caches while parsing
  // (750M file is arbitrarily considered big)
  bool bigfile;
  if ( "auto" == usecache ) {
    bigfile = !rollover && ( xmlstat.st_size > 1024 * 1024 * 750 );
  }
  else {
    bigfile = ( "true" == usecache );
  }

  if ( bigfile ) {
    std::cout << "using disk caches" << std::endl;
  }

  H5::Exception::dontPrint( );

  try {
    CacheFileReader writer( hdf5path, compression, bigfile, prefix );
    writer.convert( xmlpath );
  }
  catch ( FileIException error ) {
    std::cerr << "could not open output file" << std::endl;
    error.printError( );
    return 1;
  }// catch failure caused by the DataSet operations
  catch ( DataSetIException error ) {
    error.printError( );
    return 2;
  }// catch failure caused by the DataSpace operations
  catch ( DataSpaceIException error ) {
    error.printError( );
    return 3;
  }// catch failure caused by the DataSpace operations
  catch ( DataTypeIException error ) {
    error.printError( );
    return 4;
  }
  catch ( const std::exception& ex ) {
    std::cerr << "unhandled exception " << ex.what( ) << std::endl;
    return 5;
  }
  catch ( const std::string& ex ) {
    std::cerr << "unhandled exception " << ex << std::endl;
    return 6;
  }

  return 0;
}


