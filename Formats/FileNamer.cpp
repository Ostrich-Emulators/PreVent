/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "FileNamer.h"
#include "SignalSet.h"
#include "config.h"
#include "Log.h"
#include "Options.h"

#include <sys/stat.h>
#include <iostream>
#include <ctime>
#include <sstream>
#include <time.h>
#include <stdio.h>

#include <filesystem>

//this should fetch current working directory regardless of platform
#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

namespace FormatConverter{
  const std::string FileNamer::DEFAULT_PATTERN = "%d%i-p%p-%s.%t";
  const std::string FileNamer::FILENAME_PATTERN = "%i.%t";

  FileNamer::FileNamer( const std::string& pat ) : pattern( pat ) { }

  FileNamer::FileNamer( const FileNamer& orig ) : pattern( orig.pattern ),
      conversions( orig.conversions.begin( ), orig.conversions.end( ) ),
      lastname( orig.lastname ), inputfile( orig.inputfile ) { }

  FileNamer::~FileNamer( ) { }

  FileNamer& FileNamer::operator=(const FileNamer& orig ) {
    if ( this != &orig ) {
      pattern = orig.pattern;
      conversions.clear( );
      conversions.insert( orig.conversions.begin( ), orig.conversions.end( ) );
      lastname = orig.lastname;
      inputfile = orig.inputfile;
    }
    return *this;
  }

  FileNamer FileNamer::parse( const std::string& pattern ) {
    std::string expand = pattern;
    size_t pos = expand.find( "%S" );
    while ( pos != std::string::npos ) //initially sorts through file for all standard form flags and expands them
    {
      Log::trace( ) << "replacing %S with %i-p%p-%s.%t" << std::endl;
      expand.replace( pos, 2, "%i-p%p-%s.%t" );
      pos = expand.find( "%S", pos + 1 );
    }
    return FileNamer( expand );
  }

  void FileNamer::inputfilename( const std::string& inny ) {
    inputfile = inny;

    // FIXME: use filesystem paths in this function
    // auto path = std::filesystem::path( inny );

    //Last modification date
    struct stat t_stat;
    stat( inputfile.c_str( ), &t_stat ); //reads metadata of input file
    struct tm * timeinfo = localtime( &t_stat.st_mtime ); //stores last modification time in pointer
    conversions["%m"] = YYYYMMDD( timeinfo );

    const size_t sfxpos = inny.rfind( "." );
    std::string input = inny;
    if ( std::string::npos != sfxpos ) {
      input = inny.substr( 0, sfxpos );
      conversions["%i"] = input;
      conversions["%x"] = inny.substr( sfxpos + 1 );
    }

    // get rid of any leading directories
#ifdef __CYGWIN__
    // if cygwin, we have to handle both possibilities for a nice user experience
    auto bpos = input.rfind( "\\" );
    if ( std::string::npos == bpos ) {
      bpos = input.rfind( "/" );
    }
    const size_t basepos = bpos;
#else
    const size_t basepos = input.rfind( dirsep );
#endif

    if ( std::string::npos != basepos ) {
      conversions["%i"] = input.substr( basepos + 1 );
      conversions["%d"] = input.substr( 0, basepos ) + dirsep;
    }
    else {
      conversions["%i"] = input;
      conversions["%d"] = "";
    }
  }

  std::string FileNamer::inputfilename( ) const {
    return inputfile;
  }

  std::string FileNamer::filenameNoExt( SignalSet * data ) {
    // for now, always the same thing
    const size_t pos = lastname.rfind( "." );
    return lastname.substr( 0, pos );
  }

  std::string FileNamer::filename( SignalSet * data ) {
    // we need to have data for all the conversion keys in here

    const tm EARLY = modtime( data->earliest( ) );
    const tm LATE = modtime( data->latest( ) );

    conversions["%s"] = getDateSuffix( &EARLY, "" );
    conversions["%e"] = getDateSuffix( &LATE, "" );

    conversions["%T"] = HHmmss( &EARLY );
    conversions["%E"] = HHmmss( &LATE );

    conversions["%M"] = 0 == data->metadata( ).count( "MRN" )
        ? ""
        : data->metadata( ).at( "MRN" );

    //Current Date
    time_t tim;
    time( &tim );
    struct tm *curr_time = localtime( &tim );
    conversions["%D"] = YYYYMMDD( curr_time );

    //Current working directory
    char cCurrentPath[FILENAME_MAX];
    if ( !GetCurrentDir( cCurrentPath, sizeof (cCurrentPath ) ) ) {
      perror( "Error accessing current directory" );
    }
    conversions["%C"] = std::string( cCurrentPath ) + dirsep;

    lastname = pattern;
    const std::string replacements[] = {
      "%d",
      "%i",
      "%p",
      "%s",
      "%t",
      "%o",
      "%C",
      "%D",
      "%e",
      "%x",
      "%m",
      "%S",
      "%T",
      "%E",
      "%M"
    };

    for ( auto x : replacements ) {
      size_t pos = lastname.find( x );
      while ( pos != std::string::npos ) {
        Log::trace( ) << "replacing " << x << " with " << conversions[x] << std::endl;
        lastname.replace( pos, 2, conversions[x] );
        pos = lastname.find( x, pos + 1 );
      }
    }

    return lastname;
  }

  std::string FileNamer::filename( ) {
    lastname = filenameNoExt( );
    if ( 0 != conversions.count( "tofmt" ) ) {
      lastname += conversions.at( "tofmt" );
    }
    return lastname;
  }

  std::string FileNamer::filenameNoExt( ) {
    // for now, always the same thing
    if ( 0 == conversions.count( "%p" ) ) {
      conversions["%p"] = 1;
    }
    lastname = conversions["%d"] + dirsep + conversions["%i"] + conversions["%p"];
    return lastname;
  }

  void FileNamer::patientOrdinal( int patient ) {
    conversions["%p"] = std::to_string( patient );
  }

  void FileNamer::fileOrdinal( int fnum ) {
    conversions["%o"] = std::to_string( fnum );
  }

  void FileNamer::tofmt( const std::string& ext ) {
    conversions["%t"] = ext;
  }

  std::string FileNamer::last( ) const {
    return lastname;
  }

  std::string FileNamer::getDateSuffix( const tm * dater, const std::string& sep ) {
    return sep + YYYYMMDD( dater );
  }

  std::string FileNamer::YYYYMMDD( const tm * time ) {
    // we want YYYYMMDD format, but cygwin seems to misinterpret %m for strftime
    // so we're doing it manually (for now)
    std::stringstream ss;
    ss << ( 1900 + time->tm_year );
    if ( time->tm_mon + 1 < 10 ) ss << 0;
    ss << time->tm_mon + 1;
    if ( time->tm_mday < 10 ) ss << 0;
    ss << time->tm_mday;
    return ss.str( );
  }

  std::string FileNamer::HHmmss( const tm * time ) {
    std::stringstream ss;
    if ( time->tm_hour < 10 ) ss << 0;
    ss << time->tm_hour << "_";
    if ( time->tm_min < 10 ) ss << 0;
    ss << time->tm_min << "_";
    if ( time->tm_sec < 10 ) ss << 0;
    ss << time->tm_sec;
    return ss.str( );
  }

  tm FileNamer::modtime( const dr_time& time ) {
    time_t mytime = time / 1000;
    return *( Options::asBool( OptionsKey::LOCALIZED_TIME )
        ? std::localtime( &mytime )
        : std::gmtime( &mytime ) );
  }
}