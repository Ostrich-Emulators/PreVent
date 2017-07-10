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

#include "ZlReader.h"
#include "FromReader.h"
#include "ToWriter.h"
#include "Formats.h"
#include "SignalData.h"
#include "DataRow.h"
#include "ReadInfo.h"

#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif

using namespace std;

void helpAndExit( char * progname, std::string msg = "" ) {
  std::cerr << msg << std::endl
      << "Syntax: " << progname << " --from <format> --to <format> <file>..."
      << std::endl << "\t-f or --from <input format>"
      << std::endl << "\t-t or --to <output format>"
      << std::endl << "\t-o or --outdir <output directory>"
      << std::endl << "\t-z or --compression <compression level (0-9, default: 6)>"
      << std::endl << "\t-p or --prefix <output file prefix>"
      << std::endl << "\tValid formats: wfdb, hdf5, stpxml, zl"
      << std::endl << "\tIf file is -, stdin is read for input, and the format is assumed to be our zl format, regardless of --from option"
      << std::endl << std::endl;
  exit( 1 );
}

struct option longopts[] = {
  { "from", required_argument, NULL, 'f' },
  { "to", required_argument, NULL, 't' },
  { "outdir", required_argument, NULL, 'o' },
  { "compression", required_argument, NULL, 'z' },
  { "prefix", required_argument, NULL, 'p' },
  { 0, 0, 0, 0 }
};

int main( int argc, char** argv ) {
  char c;
  extern int optind;
  extern char * optarg;
  std::string fromstr = "";
  std::string tostr = "";
  std::string outdir = ".";
  std::string prefix = "";
  int compression = 6;

  while ( ( c = getopt_long( argc, argv, ":f:t:ozp", longopts, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'f':
        fromstr = optarg;
        break;
      case 't':
        tostr = optarg;
        break;
      case 'o':
        outdir = optarg;
        break;
      case 'p':
        prefix = optarg;
        break;
      case 'z':
        compression = std::atoi( optarg );
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
    ReadInfo data;
    std::cout << "converting " << argv[i]
        << " from " << fromstr
        << " to " << tostr << std::endl;
    from->reset( argv[i], data );
    to->setOutputDir( outdir );
    to->setCompression( compression );
    to->setOutputPrefix( prefix );
    std::vector<std::string> files = to->write( from, data );

    for ( const auto& f : files ) {
      std::cout << " written to " << f << std::endl;
    }
  }

//  H5::Exception::dontPrint( );
//
//  try {
//    ZlReader writer( hdf5path, compression, bigfile, prefix );
//    writer.convert( xmlpath );
//  }
//  catch ( FileIException error ) {
//    std::cerr << "could not open output file" << std::endl;
//    error.printError( );
//    return 1;
//  }// catch failure caused by the DataSet operations
//  catch ( DataSetIException error ) {
//    error.printError( );
//    return 2;
//  }// catch failure caused by the DataSpace operations
//  catch ( DataSpaceIException error ) {
//    error.printError( );
//    return 3;
//  }// catch failure caused by the DataSpace operations
//  catch ( DataTypeIException error ) {
//    error.printError( );
//    return 4;
//  }
//  catch ( const std::exception& ex ) {
//    std::cerr << "unhandled exception " << ex.what( ) << std::endl;
//    return 5;
//  }
//  catch ( const std::string& ex ) {
//    std::cerr << "unhandled exception " << ex << std::endl;
//    return 6;
//  }

  return 0;
}


