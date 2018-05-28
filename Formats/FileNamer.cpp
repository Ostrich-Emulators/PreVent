/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "FileNamer.h"
#include "SignalSet.h"
#include <sys/stat.h>
#include "config.h"

const std::string FileNamer::DEFAULT_PATTERN = "%d%i-p%p-%s.%t";
const std::string FileNamer::FILENAME_PATTERN = "%d%i.%t";

FileNamer::FileNamer( const std::string& pat ) : pattern( pat ) {
}

FileNamer::FileNamer( const FileNamer& orig ) : pattern( orig.pattern ),
conversions( orig.conversions.begin( ), orig.conversions.end( ) ), lastname( orig.lastname ) {
}

FileNamer::~FileNamer( ) {
}

FileNamer& FileNamer::operator=(const FileNamer& orig ) {
  if ( this != &orig ) {
    pattern = orig.pattern;
    conversions.clear( );
    conversions.insert( orig.conversions.begin( ), orig.conversions.end( ) );
    lastname = orig.lastname;
  }
  return *this;
}

FileNamer FileNamer::parse( const std::string& pattern ) {
  return FileNamer( pattern );
}

void FileNamer::inputfilename( const std::string& inny ) {
  const size_t sfxpos = inny.rfind( "." );
  std::string input = inny;
  if ( std::string::npos != sfxpos ) {
    input = inny.substr( 0, sfxpos );
    conversions["%i"] = input;
    conversions["%x"] = inny.substr( sfxpos + 1 );
  }

  // get rid of any leading directories
  const size_t basepos = input.rfind( dirsep );
  if ( std::string::npos != basepos ) {
    conversions["%i"] = input.substr( basepos + 1 );
    conversions["%d"] = input.substr( 0, basepos );
  }
  else {
    conversions["%d"] = "";
  }
}

std::string FileNamer::filenameNoExt( const SignalSet& data ) {
  // for now, always the same thing
  const size_t pos = lastname.rfind( "." );
  return lastname.substr( 0, pos );
}

std::string FileNamer::filename( const SignalSet& data ) {
  // we need to have data for all the conversion keys in here

  conversions["%s"] = getDateSuffix( data.earliest( ), "" );

  lastname = pattern;
  const std::string replacements[] = {
    "%d",
    "%i",
    "%p",
    "%s",
    "%t",
    "%o"
  };

  for ( auto x : replacements ) {
    size_t pos = lastname.find( x );

    // FIXME: what if a key is in the pattern more than once?
    if ( std::string::npos != pos ) {
      lastname.replace( pos, 2, conversions[x] );
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

void FileNamer::outputdir( const std::string& out ) {
  struct stat info;
  std::string outdir( out );
  if ( stat( outdir.c_str( ), &info ) != 0 ) {
    mkdir( outdir.c_str( ), S_IRWXU | S_IRWXG );
  }

  size_t extpos = outdir.find_last_of( dirsep, outdir.length( ) );
  if ( outdir.length( ) - 1 != extpos || extpos < 0 ) {
    // doesn't end with a dirsep, so add it
    outdir += dirsep;
  }

  conversions["outputdir"] = outdir;
}

std::string FileNamer::outputdir( ) const {
  return conversions.at( "outputdir" );
}

void FileNamer::tofmt( const std::string& ext ) {
  conversions["%t"] = ext;
}

std::string FileNamer::last( ) const {
  return lastname;
}

std::string FileNamer::getDateSuffix( const dr_time& date, const std::string& sep ) {
  time_t mytime = date / 1000;
  tm * dater = std::gmtime( &mytime );
  // we want YYYYMMDD format, but cygwin seems to misinterpret %m for strftime
  // so we're doing it manually (for now)
  std::string ret = sep;
  ret += std::to_string( dater->tm_year + 1900 );

  if ( dater->tm_mon < 10 ) {
    ret += '0';
  }
  ret += std::to_string( dater->tm_mon );

  if ( dater->tm_mday < 10 ) {
    ret += '0';
  }
  ret += std::to_string( dater->tm_mday );

  return ret;
}
