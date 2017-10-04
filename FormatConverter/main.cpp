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

#include "ZlReader.h"
#include "Reader.h"
#include "Writer.h"
#include "Formats.h"
#include "SignalData.h"
#include "DataRow.h"
#include "SignalSet.h"
#include "Db.h"

void helpAndExit( char * progname, std::string msg = "" ) {
  std::cerr << msg << std::endl
      << "Syntax: " << progname << " --to <format> <file>..."
      << std::endl << "\t-f or --from <input format>"
      << std::endl << "\t-t or --to <output format>"
      << std::endl << "\t-o or --outdir <output directory>"
      << std::endl << "\t-z or --compression <compression level (0-9, default: 6)>"
      << std::endl << "\t-p or --prefix <output file prefix>"
      << std::endl << "\t-e or --export <vital/wave to export>"
      << std::endl << "\t-s or --sqlite <db file>"
      << std::endl << "\t-q or --quiet"
      << std::endl << "\tValid input formats: wfdb, hdf5, stpxml, cpcxml"
      << std::endl << "\tValid output formats: wfdb, hdf5, mat, csv"
      << std::endl << "\tthe --sqlite option will create/add metadata to a sqlite database"
      //<< std::endl << "\tIf file is -, stdin is read for input, and the format is assumed to be our zl format, regardless of --from option"
      << std::endl << std::endl;
  exit( 1 );
}

struct option longopts[] = {
  { "from", required_argument, NULL, 'f' },
  { "to", required_argument, NULL, 't' },
  { "outdir", required_argument, NULL, 'o' },
  { "compression", required_argument, NULL, 'z' },
  { "prefix", required_argument, NULL, 'p' },
  { "export", required_argument, NULL, 'e' },
  { "sqlite", required_argument, NULL, 's' },
  { "quiet", no_argument, NULL, 'q' },
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
  std::string exp = "";
  std::string sqlitedb = "";
  bool quiet = false;
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
      case 'e':
        exp = optarg;
        break;
      case 'q':
        quiet = true;
        break;
      case 's':
        sqlitedb = optarg;
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
    Format f = Formats::guess( argv[optind] );
    switch ( f ) {
      case HDF5:
        fromstr = "hdf5";
        break;
      case WFDB:
        fromstr = "wfdb";
        break;
      case MAT5:
        fromstr = "mat5";
        break;
      case STPXML:
        fromstr = "stpxml";
        break;
      case DSZL:
        fromstr = "zl";
        break;
    }
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

  std::unique_ptr<Reader> from;
  std::unique_ptr<Writer> to;
  try {
    from = Reader::get( fromfmt );
    to = Writer::get( tofmt );
    to->setQuiet( quiet );
    from->setQuiet( quiet );

    if ( !exp.empty( ) ) {
      from->extractOnly( exp );
    }
  }
  catch ( std::string x ) {
    std::cerr << x << std::endl;
  }

  std::shared_ptr<Db> db;
  if ( !sqlitedb.empty( ) ) {
    db.reset( new Db( ) );
    db->init( sqlitedb );
    to->addListener( db );

    db->setProperty( ConversionProperty::QUIET, ( quiet ? "TRUE" : "FALSE" ) );
  }

  int returncode = 0;
  // send the files through
  for ( int i = optind; i < argc; i++ ) {
    SignalSet data;
    std::cout << "converting " << argv[i]
        << " from " << fromstr
        << " to " << tostr << std::endl;
    to->setOutputDir( outdir );
    to->setCompression( compression );
    to->setOutputPrefix( prefix );

    if ( from->prepare( argv[i], data ) < 0 ) {
      std::cerr << "could not prepare file for reading" << std::endl;
      returncode = -1;
      continue;
    }
    else {
      std::vector<std::string> files = to->write( from, data );
      from->finish( );

      for ( const auto& f : files ) {
        std::cout << " written to " << f << std::endl;
      }

      if ( db ) {
        db->onConversionCompleted( argv[i], files );
      }
    }
  }

  return returncode;
}


