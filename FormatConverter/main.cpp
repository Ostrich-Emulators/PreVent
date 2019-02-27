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
#include "BasicSignalSet.h"
#include "Db.h"
#include "config.h"
#include "FileNamer.h"
#include "TimezoneOffsetTimeSignalSet.h"
#include "AnonymizingSignalSet.h"
#include "Options.h"

#include "build.h"

void helpAndExit( char * progname, std::string msg = "" ) {
  std::cerr << msg << std::endl
      << "Syntax: " << progname << " --to <format> <file>..."
      << std::endl << "\t-f or --from <input format>"
      << std::endl << "\t-t or --to <output format>"
      << std::endl << "\t-z or --compression <compression level (0-9, default: 6)>"
      << std::endl << "\t-s or --sqlite <db file>"
      << std::endl << "\t-q or --quiet"
      << std::endl << "\t-1 or --stop-after-one"
      << std::endl << "\t-l or --localtime"
      << std::endl << "\t-p or --pattern <naming pattern>"
      << std::endl << "\t-n or --no-break or --one-file"
      << std::endl << "\t-C or --no-cache"
      << std::endl << "\t-T or --time-step (store timing information as offset from start of file"
      << std::endl << "\t-a or --anonymize, --anon, or --anonymous"
      << std::endl << "\tValid input formats: wfdb, hdf5, stpxml, cpcxml, stpjson, tdms, medi, zl"
      << std::endl << "\tValid output formats: wfdb, hdf5, mat, csv"
      << std::endl << "\tthe --sqlite option will create/add metadata to a sqlite database"
      << std::endl << "\tthe --pattern option recognizes these format specifiers:"
      << std::endl << "\t  %p - patient ordinal"
      << std::endl << "\t  %i - input filename (without directory or extension)"
      << std::endl << "\t  %d - input directory (with trailing separator)"
      << std::endl << "\t  %C - current directory (with trailing separator)"
      << std::endl << "\t  %x - input extension"
      << std::endl << "\t  %m - modified date of input file"
      << std::endl << "\t  %c - creation date of input file"
      << std::endl << "\t  %D - date of conversion"
      << std::endl << "\t  %s - date of first data point"
      << std::endl << "\t  %e - date of last data point"
      << std::endl << "\t  %o - output file ordinal"
      << std::endl << "\t  %t - the --to option's extension (e.g., hdf5, csv)"
      << std::endl << "\t  all dates are output in YYYYMMDD format"
      << std::endl << "\tthe --no-break option will ignore end of day/end of patient events, and name the output file(s) from the input file (or pattern)"
      << std::endl << "\tif file is -, stdin is read for input"
      << std::endl << std::endl;
  exit( 1 );
}

struct option longopts[] = {
  { "from", required_argument, NULL, 'f' },
  { "to", required_argument, NULL, 't' },
  { "compression", required_argument, NULL, 'z' },
  { "sqlite", required_argument, NULL, 's' },
  { "quiet", no_argument, NULL, 'q' },
  { "anonymize", no_argument, NULL, 'a' },
  { "anon", no_argument, NULL, 'a' },
  { "anonymous", no_argument, NULL, 'a' },
  { "no-break", no_argument, NULL, 'n' },
  { "one-file", no_argument, NULL, 'n' },
  { "pattern", required_argument, NULL, 'p' },
  { "localtime", no_argument, NULL, 'l' },
  { "local", no_argument, NULL, 'l' },
  { "stop-after-one", no_argument, NULL, '1' },
  { "no-cache", no_argument, NULL, 'C' },
  { "time-step", no_argument, NULL, 'T' },
  { 0, 0, 0, 0 }
};

int main( int argc, char** argv ) {
  char c;
  extern int optind;
  extern char * optarg;
  std::string fromstr;
  std::string tostr;
  std::string sqlitedb;
  std::string pattern = FileNamer::DEFAULT_PATTERN;
  bool anonymize = false;
  bool quiet = false;
  bool nobreak = false;
  bool dolocaltime = false;
  bool stopatone = false;
  int compression = Writer::DEFAULT_COMPRESSION;

  while ( ( c = getopt_long( argc, argv, ":f:t:o:z:p:s:qanl1CT", longopts, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'f':
        fromstr = optarg;
        break;
      case 't':
        tostr = optarg;
        break;
      case 'z':
        compression = std::atoi( optarg );
        break;
      case 'q':
        quiet = true;
        Options::set( OptionsKey::QUIET, true );
        break;
      case 'T':
        Options::set( OptionsKey::TIMESTEPS, true );
        break;
      case 's':
        sqlitedb = optarg;
        break;
      case 'p':
        pattern = optarg;
        break;
      case 'a':
        anonymize = true;
        break;
      case 'l':
        dolocaltime = true;
        break;
      case 'C':
        Options::set( OptionsKey::NOCACHE, true );
        break;
      case 'n':
        nobreak = true;
        if ( FileNamer::DEFAULT_PATTERN == pattern ) {
          pattern = FileNamer::FILENAME_PATTERN;
        }
        break;
      case '1':
        stopatone = true;
        break;
      case ':':
        std::cerr << "missing option argument" << std::endl;
        helpAndExit( argv[0] );
        break;
      case '?':
      default:
        helpAndExit( argv[0] );
        break;
    }
  }

  if ( !quiet ) {
    std::cout << argv[0] << " version " << FC_VERS_MAJOR
        << "." << FC_VERS_MINOR << "." << FC_VERS_MICRO
        << "; build " << GIT_BUILD << std::endl;

  }

  if ( ( optind + 1 ) > argc ) {
    helpAndExit( argv[0], "no file specified" );
  }

  const std::string argument( argv[optind] );
  if ( "-" == argument ) {
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
      case MAT4:
        fromstr = "mat4";
        break;
      case MAT73:
        fromstr = "mat73";
        break;
      case STPXML:
        fromstr = "stpxml";
        break;
      case DSZL:
        fromstr = "zl";
        break;
      case STPJSON:
        fromstr = "stpjson";
        break;
      case TDMS:
        fromstr = "tdms";
        break;
      case CSV:
        fromstr = "csv";
        break;
      case CPCXML:
        fromstr = "cpcxml";
        break;
      case UNRECOGNIZED:
        fromstr = "";
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
    to->quiet( quiet );
    to->compression( compression );
    to->stopAfterFirstFile( stopatone );
    FileNamer namer = FileNamer::parse( pattern );
    namer.tofmt( to->ext( ) );
    to->filenamer( namer );

    from->setQuiet( quiet );
    from->setNonbreaking( nobreak );
    from->localizeTime( dolocaltime );
  }
  catch ( std::string x ) {
    std::cerr << x << std::endl;
  }

  std::shared_ptr<Db> db;
  if ( !sqlitedb.empty( ) ) {
    db.reset( new Db( ) );
    db->init( sqlitedb );
    to->addListener( db );
  }

  int returncode = 0;
  // send the files through
  for ( int i = optind; i < argc; i++ ) {
    // see if the file exists before we do anything
    struct stat buffer;
    if ( 0 != stat( argv[i], &buffer ) ) {
      std::cerr << "could not open file: " << argv[i] << std::endl;
      continue;
    }

    std::unique_ptr<SignalSet>data( new BasicSignalSet( ) );
    if ( dolocaltime ) {
      data.reset( new TimezoneOffsetTimeSignalSet( data.release( ) ) );
    }
    if ( anonymize ) {
      data.reset( new AnonymizingSignalSet( data.release( ), to->filenamer( ) ) );
    }

    std::string input( argv[i] );
    to->filenamer( ).inputfilename( input );
    std::cout << "converting " << input
        << " from " << fromstr
        << " to " << tostr << std::endl;

    if ( from->prepare( input, data ) < 0 ) {
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


