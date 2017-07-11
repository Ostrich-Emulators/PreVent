
#include "ToWriter.h"
#include <sys/stat.h>
#include <iostream>

#include "Hdf5Writer.h"
#include "FromReader.h"
#include "config.h"

ToWriter::ToWriter( ) {
}

ToWriter::ToWriter( const ToWriter& ) {

}

ToWriter::~ToWriter( ) {
}

std::unique_ptr<ToWriter> ToWriter::get( const Format& fmt ) {
  switch ( fmt ) {
    case HDF5:
      return std::unique_ptr<ToWriter>( new Hdf5Writer( ) );
  }
}

void ToWriter::setOutputPrefix( const std::string& pre ) {
  prefix = pre;
}

void ToWriter::setCompression( int lev ) {
  compression = lev;
}

void ToWriter::setOutputDir( const std::string& _outdir ) {
  outdir = _outdir;

  struct stat info;
  if ( stat( outdir.c_str( ), &info ) != 0 ) {
    mkdir( outdir.c_str( ), S_IRWXU | S_IRWXG );
  }

  size_t extpos = outdir.find_last_of( dirsep, outdir.length( ) );
  if ( outdir.length( ) - 1 != extpos || extpos < 0 ) {
    // doesn't end with a dirsep, so add it
    outdir += dirsep;
  }
}

std::vector<std::string> ToWriter::write( std::unique_ptr<FromReader>& from,
    ReadInfo& data ) {
  ReadResult retcode = from->fill( data );
  std::vector<std::string> list;

  std::string lastPatientName = "";
  int patientno = 1;

  initDataSet( outdir + prefix + "-p" + std::to_string( patientno ), compression );

  while ( retcode != ReadResult::ERROR ) {
    drain( data );
    if ( ReadResult::END_OF_PATIENT == retcode ) {
      // end of old patient
      std::string file = closeDataSet( );
      if ( file.empty( ) ) {
        std::cerr << "refusing to write empty data file!" << std::endl;
      }
      else {
        list.push_back( file );
      }

      // if our patient name changed, increment our patient number
      if ( 0 != data.metadata( ).count( "Patient Name" ) &&
          data.metadata( )["Patient Name"] != lastPatientName ) {
        patientno++;
        lastPatientName = data.metadata( )["Patient Name"];
      }

      data.reset( false );
      initDataSet( outdir + prefix + "-p" + std::to_string( patientno ), compression );
    }
    else if ( ReadResult::END_OF_FILE == retcode ) {
      // end of file, so break out of our write

      std::string file = closeDataSet( );
      if ( file.empty( ) ) {
        std::cerr << "refusing to write empty data file!" << std::endl;
      }
      else {
        list.push_back( file );
      }
      break;
    }

    // carry on with next data chunk
    retcode = from->fill( data );
  }

  return list;
}

