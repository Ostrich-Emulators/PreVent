#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <getopt.h>

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>

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
#include "TimeParser.h"

#include "config.h"
#include "releases.h"
#include "Log.h"

using namespace FormatConverter;

void intro( char * progname ) {
  Log::info( ) << progname << " version " << FC_VERS_MAJOR
      << "." << FC_VERS_MINOR << "." << FC_VERS_MICRO
      << "; build " << GIT_BUILD << std::endl;
}

void helpAndExit( char * progname, std::string msg = "" ) {
  intro( progname );

  Log::error( ) << msg << std::endl
      << "Syntax: " << progname << " --to <format> <file>..."
      << std::endl << "\t-f or --from <input format>"
      << std::endl << "\t-t or --to <output format>"
      << std::endl << "\t-z or --compression <compression level (0-9, default: 6)>"
      << std::endl << "\t-s or --sqlite <db file>"
      << std::endl << "\t-q or --quiet (repeat for quieter)"
      << std::endl << "\t-v or --verbose (repeat for noisier)"
      << std::endl << "\t-1 or --stop-after-one"
      << std::endl << "\t-l or --localtime"
      << std::endl << "\t-Z or --offset <time string (MM/DD/YYYY) or seconds since 01/01/1970>"
      << std::endl << "\t-S or --opening-date <time string (MM/DD/YYYY) or seconds since 01/01/1970>"
      << std::endl << "\t-D or --skip-to-date <time string (MM/DD/YYYYTHH:MM:SS)>"
      << std::endl << "\t-p or --pattern <naming pattern>"
      << std::endl << "\t-x or --split <m[idnight]|0-24[h]>"
      << std::endl << "\t-n or --no-break or --one-file (convenience for --split 0)"
      << std::endl << "\t-C or --no-cache"
      << std::endl << "\t-T or --time-step (store timing information as offset from start of file"
      << std::endl << "\t-a or --anonymize, --anon, or --anonymous"
      << std::endl << "\t-w or --skip-waves"
      << std::endl << "\t-m or --tmpdir <directory>"
      << std::endl << "\t-R or --release (show release information and exit)"
      << std::endl << "\t-d or --allow-duplicate-filenames"
      << std::endl << "\t-X or --experimental"
      << std::endl << "\tValid input formats: wfdb, hdf5, stpxml, stpge, stpp, cpcxml, stpjson, tdms, medi, dwc, zl, csv, csv2, dwcx"
      << std::endl << "\tValid output formats: wfdb, hdf5, mat, csv, au"
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
      << std::endl << "\t  %T - time of first data point (24hr clock)"
      << std::endl << "\t  %E - time of last data point (24hr clock)"
      << std::endl << "\t  %o - output file ordinal"
      << std::endl << "\t  %M - value of the MRN metadata attribute"
      << std::endl << "\t  %t - the --to option's extension (e.g., hdf5, csv)"
      << std::endl << "\t  %S - same as %d%i-p%p-%s.%t"
      << std::endl << "\t  all dates are output in YYYYMMDD format"
      << std::endl << "\tthe --no-break option will ignore end of day/end of patient events, and name the output file(s) from the input file (or pattern)"
      << std::endl << "\tthe --offset option will shift dates by the desired amount"
      << std::endl << "\tthe --opening-date option will shift dates so that the first time in the output is the given date"
      << std::endl << "\tthe --skip-to-date option skip data collection until the given date is found. Not all readers support this option"
      << std::endl << "\tif file is -, stdin is read for input"
      << std::endl << std::endl;
  exit( 1 );
}

SplitLogic resolveSplitDurationAndNamingPattern( char * progname,
    const std::string& duration, std::string& pattern ) {
  if ( "0" == duration ) {
    Options::set( OptionsKey::NO_BREAK );
    if ( FileNamer::DEFAULT_PATTERN == pattern ) {
      pattern = FileNamer::FILENAME_PATTERN;
    }
    Log::debug( ) << "not splitting files" << std::endl;
    return SplitLogic::nobreaks( );
  }

  if ( 0 == duration.find( 'm' ) ) {
    Log::debug( ) << "splitting files at midnight" << std::endl;
    return SplitLogic::midnight( );
  }

  try {
    bool clean = ( duration.find( 'h' ) != std::string::npos );
    int hours = std::stoi( duration );
    Log::debug( ) << "splitting files every " << hours << " hours";
    if ( clean ) {
      Log::debug( ) << " at the top of the hour";
    }
    Log::debug( ) << std::endl;

    return SplitLogic::hourly( hours, clean );
  }
  catch ( std::exception& x ) {
    helpAndExit( progname, "unable to parse split duration: " + duration );
  }

  // we should never get here, but avoid a compiler warning
  return SplitLogic::midnight( );
}

void settmpdir( std::string tmpdir ) {
#ifdef __CYGWIN__
  size_t size = cygwin_conv_path( CCP_WIN_A_TO_POSIX | CCP_RELATIVE, tmpdir.c_str( ), NULL, 0 );
  if ( size < 0 ) {
    Log::error( ) << "cannot resolve path: " << tmpdir << std::endl;
    exit( 2 );
  }

  char * cygpath = (char *) malloc( size );
  if ( cygwin_conv_path( CCP_WIN_A_TO_POSIX | CCP_RELATIVE, tmpdir.c_str( ), cygpath, size ) ) {
    Log::error( ) << "error converting path!" << std::endl;
    perror( "cygwin_conv_path" );
    exit( 2 );
  }
  tmpdir = cygpath;
#endif

  setenv( "TMP", tmpdir.c_str( ), 1 );
  setenv( "TMPDIR", tmpdir.c_str( ), 1 );
  Options::set( OptionsKey::TMPDIR, tmpdir );

  // make sure the path exists (just in case!)
  if ( !std::filesystem::exists( tmpdir ) ) {
    try {
      std::filesystem::create_directories( tmpdir );
    }
    catch ( std::runtime_error& x ) {
      Log::error( ) << "could not set tmpdir: " << x.what( ) << std::endl;
      exit( 2 );
    }
  }

#ifdef __CYGWIN__
  free( cygpath );
#endif
}

struct option longopts[] = {
  { "from", required_argument, NULL, 'f' },
  { "to", required_argument, NULL, 't' },
  { "compression", required_argument, NULL, 'z' },
  { "sqlite", required_argument, NULL, 's' },
  { "quiet", no_argument, NULL, 'q' },
  { "verbose", optional_argument, NULL, 'v' },
  { "anonymize", no_argument, NULL, 'a' },
  { "anon", no_argument, NULL, 'a' },
  { "anonymous", no_argument, NULL, 'a' },
  { "no-break", no_argument, NULL, 'n' },
  { "one-file", no_argument, NULL, 'n' },
  { "pattern", required_argument, NULL, 'p' },
  { "localtime", no_argument, NULL, 'l' },
  { "local", no_argument, NULL, 'l' },
  { "offset", required_argument, NULL, 'Z' },
  { "opening-date", required_argument, NULL, 'S' },
  { "stop-after-one", no_argument, NULL, '1' },
  { "no-cache", no_argument, NULL, 'C' },
  { "release", no_argument, NULL, 'R' },
  { "time-step", no_argument, NULL, 'T' },
  { "skip-waves", no_argument, NULL, 'w' },
  { "tmpdir", required_argument, NULL, 'm' },
  { "allow-duplicate-filenames", no_argument, NULL, 'd' },
  { "split", required_argument, NULL, 'x' },
  { "skip-to-date", required_argument, NULL, 'D' },
  { "experimental", optional_argument, NULL, 'X' },
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
  std::string offsetstr;
  std::string skipstr;
  std::string experimentalstr;
  bool offsetIsDesiredDate = false;
  Options::set( OptionsKey::ANONYMIZE, false );
  Options::set( OptionsKey::INDEXED_TIME, false );
  Options::set( OptionsKey::LOCALIZED_TIME, false );
  Options::set( OptionsKey::NO_BREAK, false );
  auto loglevel = static_cast<int> ( LogLevel::INFO );
  bool stopatone = false;
  auto compression = Writer::DEFAULT_COMPRESSION;
  auto split = std::string{ "midnight" };
  auto allowdupenames = false;

  settmpdir( std::filesystem::temp_directory_path( ) );

  while ( ( c = getopt_long( argc, argv, ":f:t:o:z:p:s:q::v::anl1CdTZ:S:Rwm:x:D:X::", longopts, NULL ) ) != -1 ) {
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
        loglevel--;
        if ( nullptr != optarg ) {
          auto vvv = std::string( optarg );
          loglevel -= vvv.length( );
        }
        if ( loglevel< static_cast<int> ( LogLevel::NONE ) ) {
          loglevel = static_cast<int> ( LogLevel::NONE );
        }
        Log::setlevel( static_cast<LogLevel> ( loglevel ) );
        break;
      case 'v':
        loglevel++;
        if ( nullptr != optarg ) {
          auto vvv = std::string( optarg );
          loglevel += vvv.length( );
        }
        if ( loglevel> static_cast<int> ( LogLevel::ALL ) ) {
          loglevel = static_cast<int> ( LogLevel::ALL );
        }
        Log::setlevel( static_cast<LogLevel> ( loglevel ) );

        break;
      case 'd':
        allowdupenames = true;
        break;
      case 'T':
        Options::set( OptionsKey::INDEXED_TIME );
        break;
      case 'w':
        Options::set( OptionsKey::SKIP_WAVES );
        break;
      case 's':
        sqlitedb = optarg;
        break;
      case 'p':
        pattern = optarg;
        break;
      case 'Z':
        offsetstr = optarg;
        offsetIsDesiredDate = false;
        break;
      case 'm':
        settmpdir( optarg );
        break;
      case 'S':
        offsetstr = optarg;
        offsetIsDesiredDate = true;
        break;
      case 'l':
        Options::set( OptionsKey::LOCALIZED_TIME );
        break;
      case 'C':
        Options::set( OptionsKey::NOCACHE );
        break;
      case 'D':
        skipstr = optarg;
        break;
      case 'R':
        intro( argv[0] );
        Log::info( ) << releases_h_in << std::endl;
        exit( 0 );
        break;
      case 'a':
        Options::set( OptionsKey::ANONYMIZE );
        if ( offsetstr.empty( ) ) {
          // if no offset has (yet) been specified, anonymous files start at 01/01/1970
          offsetstr = "0";
          offsetIsDesiredDate = true;
        }
        break;
      case 'x':
        split = std::string( optarg );
        break;
      case 'X':
        experimentalstr = nullptr == optarg ? "Y" : std::string( optarg );
        break;
      case 'n':
        split = "0";
        break;
      case '1':
        stopatone = true;
        break;
      case ':':
        Log::error( ) << "missing option argument" << std::endl;
        helpAndExit( argv[0] );
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

  intro( argv[0] );

  const std::string argument( argv[optind] );
  if ( "-" == argument ) {
    fromstr = "zl";
  }

  if ( "" != skipstr ) {
    auto offset = static_cast<dr_time> ( TimeParser::parse( skipstr,
        Options::asBool( OptionsKey::LOCALIZED_TIME ) ) * 1000 );

    if ( 0 == offset ) {
      helpAndExit( argv[0], "unparseable date:" + skipstr );
    }
    else {
      Log::info( ) << "skipping data until " << skipstr << " (" << offset << ")" << std::endl;
      Options::set( OptionsKey::SKIP_UNTIL_DATETIME, offset );
    }
  }

  auto splitlogic = resolveSplitDurationAndNamingPattern( argv[0], split, pattern );

  if ( "" == fromstr ) {
    auto f = Formats::guess( argv[optind] );
    switch ( f ) {
      case FormatConverter::HDF5:
        fromstr = "hdf5";
        break;
      case FormatConverter::WFDB:
        fromstr = "wfdb";
        break;
      case FormatConverter::MAT5:
        fromstr = "mat5";
        break;
      case FormatConverter::MAT4:
        fromstr = "mat4";
        break;
      case FormatConverter::MAT73:
        fromstr = "mat73";
        break;
      case FormatConverter::STPGE:
        fromstr = "stpge";
        break;
      case FormatConverter::STPP:
        fromstr = "stpp";
        break;
      case FormatConverter::STPXML:
        fromstr = "stpxml";
        break;
      case FormatConverter::DSZL:
        fromstr = "zl";
        break;
      case FormatConverter::STPJSON:
        fromstr = "stpjson";
        break;
      case FormatConverter::MEDI:
        fromstr = "tdms";
        break;
      case FormatConverter::CSV:
        fromstr = "csv";
        break;
      case FormatConverter::CSV2:
        fromstr = "csv2";
        break;
      case FormatConverter::CPCXML:
        fromstr = "cpcxml";
        break;
      case FormatConverter::DWC:
        fromstr = "dwc";
        break;
      case FormatConverter::AUTON:
        fromstr = "au";
        break;
      case FormatConverter::DWCX:
      case FormatConverter::NOOP:
      case FormatConverter::UNRECOGNIZED:
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
  auto fromfmt = Formats::getValue( fromstr );
  auto tofmt = Formats::getValue( tostr );
  if ( Format::UNRECOGNIZED == fromfmt ) {
    helpAndExit( argv[0], "--from format not recognized" );
  }
  if ( Format::UNRECOGNIZED == tofmt ) {
    helpAndExit( argv[0], "--to format not recognized" );
  }

  // FIXME: if we're localizing time, then offsetstr should be assumed to be localtime
  // our offsetstr is supposed to be in seconds, but we want to calculate
  // our times in ms, so * 1000
  auto offset = static_cast<dr_time> ( TimeParser::parse( offsetstr,
      Options::asBool( OptionsKey::LOCALIZED_TIME ) ) * 1000 );
  auto timemod = ( offsetIsDesiredDate
      ? TimeModifier::time( offset )
      : TimeModifier::offset( offset ) );

  std::unique_ptr<Reader> from;
  std::unique_ptr<Writer> to;
  try {
    from = Reader::get( fromfmt, experimentalstr );
    to = Writer::get( tofmt );
    to->compression( compression );
    to->stopAfterFirstFile( stopatone );
    FileNamer namer = FileNamer::parse( pattern );
    namer.allowDuplicates( allowdupenames );
    namer.tofmt( to->ext( ) );
    to->filenamer( namer );

    from->localizeTime( Options::asBool( OptionsKey::LOCALIZED_TIME ) );
    from->skipToTime( Options::asTime( OptionsKey::SKIP_UNTIL_DATETIME ) );
    from->timeModifier( timemod );
    from->splitter( splitlogic );
  }
  catch ( std::string x ) {
    Log::error( ) << x << std::endl;
  }

  std::shared_ptr<Db> db;
  if ( !sqlitedb.empty( ) ) {
    db.reset( new Db( ) );
    db->init( sqlitedb );
    to->addListener( db );
  }

  int returncode = 0;
  // send the files through
  for ( auto i = optind; i < argc; i++ ) {
    // see if the file exists before we do anything
    struct stat buffer;
    if ( 0 != stat( argv[i], &buffer ) ) {
      Log::error( ) << "could not open file: " << argv[i] << std::endl;
      returncode = -1;
      continue;
    }

    auto data = std::unique_ptr<SignalSet>{ std::make_unique<BasicSignalSet>( ) };
    if ( Options::asBool( OptionsKey::ANONYMIZE ) ) {
      data.reset( new AnonymizingSignalSet( data.release( ), to->filenamer( ), timemod ) );
    }
    else if ( Options::asBool( OptionsKey::LOCALIZED_TIME ) ) {
      data.reset( new TimezoneOffsetTimeSignalSet( data.release( ) ) );
    }

    auto input( argv[i] );
    to->filenamer( ).inputfilename( input );
    Log::info( ) << "converting " << input
        << " from " << fromstr
        << " to " << tostr << std::endl;

    if ( allowdupenames ) {
      Log::warn( ) << "allowing generation of duplicate filenames (possible data loss)" << std::endl;
    }

    if ( from->prepare( input, data.get( ) ) < 0 ) {
      Log::error( ) << "could not prepare file for reading" << std::endl;
      returncode = -2;
      continue;
    }
    else {
      bool iserr;
      auto files = to->write( from.get( ), data.get( ), &iserr );
      from->finish( );

      if ( iserr ) {
        Log::error( ) << "Cannot continue after error" << std::endl;
        returncode = -3;

        for ( const auto& f : files ) {
          Log::info( ) << " (incomplete dataset) written to " << f << std::endl;
        }
      }
      else {
        for ( const auto& f : files ) {
          Log::info( ) << " written to " << f << std::endl;
        }

        if ( db ) {
          db->onConversionCompleted( argv[i], files );
        }
      }
    }
  }

  return returncode;
}
