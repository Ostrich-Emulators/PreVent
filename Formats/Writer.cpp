
#include "Writer.h"
#include <sys/stat.h>
#include <iostream>
#include <sstream>

#include "Reader.h"
#include "config.h"
#include "Hdf5Writer.h"
#include "WfdbWriter.h"
#include "ZlWriter.h"
#include "MatWriter.h"
#include "CsvWriter.h"
#include "ConversionListener.h"
#include "FileNamer.h"

Writer::Writer( const std::string& ext ) : quiet( false ), extension( ext ),
namer( new FileNamer( FileNamer::parse( FileNamer::DEFAULT_PATTERN ) ) ) {
}

Writer::Writer( const Writer& w ) : quiet( false ), extension( w.extension ),
namer( new FileNamer( FileNamer::parse( FileNamer::DEFAULT_PATTERN ) ) ) {
}

Writer::~Writer( ) {
}

std::unique_ptr<Writer> Writer::get( const Format& fmt ) {
  switch ( fmt ) {
    case HDF5:
      return std::unique_ptr<Writer>( new Hdf5Writer( ) );
    case WFDB:
      return std::unique_ptr<Writer>( new WfdbWriter( ) );
    case DSZL:
      return std::unique_ptr<Writer>( new ZlWriter( ) );
    case MAT73:
      return std::unique_ptr<Writer>( new MatWriter( MatVersion::MV7 ) );
    case MAT5:
      return std::unique_ptr<Writer>( new MatWriter( MatVersion::MV5 ) );
    case MAT4:
      return std::unique_ptr<Writer>( new MatWriter( MatVersion::MV4 ) );
    case CSV:
      return std::unique_ptr<Writer>( new CsvWriter( ) );

    default:
      throw "writer not yet implemented";
  }
}

const std::string& Writer::ext( ) const {

  return extension;
}

void Writer::setCompression( int lev ) {

  compression = lev;
}

int Writer::initDataSet( int ) {

  return 0;
}

FileNamer& Writer::filenamer( ) const {

  return *namer.get( );
}

std::vector<std::string> Writer::write( std::unique_ptr<Reader>& from,
    SignalSet& data ) {
  int patientno = 1;

  output( ) << "init data set" << std::endl;
  namer->patientOrdinal( patientno );
  int initrslt = initDataSet( compression );
  std::vector<std::string> list;
  if ( initrslt < 0 ) {
    std::cerr << "cannot init dataset: " + namer->last( ) << std::endl;
    return list;
  }

  output( ) << "filling data" << std::endl;
  ReadResult retcode = from->fill( data );

  while ( retcode != ReadResult::ERROR ) {
    drain( data );

    if ( ReadResult::END_OF_DAY == retcode || ReadResult::END_OF_PATIENT == retcode ) {
      std::vector<std::string> files = closeDataSet( );
      for ( auto& outfile : files ) {
        for ( auto& l : listeners ) {
          l->onFileCompleted( outfile, data );
        }
      }

      if ( files.empty( ) ) {
        std::cerr << "refusing to write empty data file!" << std::endl;
      }
      else {
        list.insert( list.end( ), files.begin( ), files.end( ) );
      }

      if ( ReadResult::END_OF_PATIENT == retcode ) {
        patientno++;
      }

      data.reset( false );
      output( ) << "init data set" << std::endl;
      namer->patientOrdinal( patientno );
      initDataSet( compression );
    }
    else if ( ReadResult::END_OF_FILE == retcode ) {
      // end of file, so break out of our write

      std::vector<std::string> files = closeDataSet( );
      if ( files.empty( ) ) {
        std::cerr << "refusing to write empty data file!" << std::endl;
      }
      else {
        list.insert( list.end( ), files.begin( ), files.end( ) );

        for ( auto& outfile : files ) {
          for ( auto& l : listeners ) {
            l->onFileCompleted( outfile, data );
          }
        }
      }
      break;
    }

    // carry on with next data chunk
    output( ) << "reading next file chunk" << std::endl;
    retcode = from->fill( data, retcode );
  }

  if ( ReadResult::ERROR == retcode ) {

    std::cerr << "error reading file" << std::endl;
  }

  return list;
}

void Writer::addListener( std::shared_ptr<ConversionListener> l ) {

  listeners.push_back( l );
}

class NullBuffer : public std::streambuf{
public:

  int overflow( int c ) {

    return c;
  }
};

void Writer::setQuiet( bool q ) {

  quiet = q;
}

void Writer::filenamer( const FileNamer& p ) {

  namer.reset( new FileNamer( p ) );
}

std::ostream& Writer::output( ) const {
  return ( quiet ? ( std::ostream& ) ss : std::cout );
}
