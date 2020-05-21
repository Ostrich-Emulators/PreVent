
#include "Writer.h"
#include <sys/stat.h>
#include <iostream>
#include <sstream>

#include "Reader.h"
#include "config.h"
#include "Hdf5Writer.h"
#include "WfdbWriter.h"
#include "MatWriter.h"
#include "CsvWriter.h"
#include "NullWriter.h"
#include "ConversionListener.h"
#include "FileNamer.h"
#include "OffsetTimeSignalSet.h"

namespace FormatConverter{

  const int Writer::DEFAULT_COMPRESSION = 6;

  Writer::Writer( const std::string& ext ) : compress( DEFAULT_COMPRESSION ),
      bequiet( false ), extension( ext ),
      namer( std::make_unique<FileNamer>( FileNamer::parse( FileNamer::DEFAULT_PATTERN ) ) ) {
    // figure out a string for our timezone by getting a reference time
    time_t reftime = std::time( nullptr );
    tm * reftm = localtime( &reftime );
    gmt_offset = reftm->tm_gmtoff;
    timezone = std::string( reftm->tm_zone );
  }

  Writer::Writer( const Writer& w ) : compress( DEFAULT_COMPRESSION ), bequiet( false ),
      extension( w.extension ), namer( std::make_unique<FileNamer>( FileNamer::parse( FileNamer::DEFAULT_PATTERN ) ) ),
      gmt_offset( w.gmt_offset ), timezone( w.timezone ) { }

  Writer::~Writer( ) { }

  std::unique_ptr<Writer> Writer::get( const FormatConverter::Format& fmt ) {
    switch ( fmt ) {
      case FormatConverter::HDF5:
        return std::make_unique<Hdf5Writer>( );
      case FormatConverter::WFDB:
        return std::make_unique<WfdbWriter>( );
        //case DSZL:
        //return std::make_unique<ZlWriter>();
      case FormatConverter::MAT73:
        return std::make_unique<MatWriter>( MatVersion::MV7 );
      case FormatConverter::MAT5:
        return std::make_unique<MatWriter>( MatVersion::MV5 );
      case FormatConverter::MAT4:
        return std::make_unique<MatWriter>( MatVersion::MV4 );
      case FormatConverter::CSV:
        return std::make_unique<CsvWriter>( );
      case FormatConverter::NOOP:
        return std::make_unique<NullWriter>( );
      default:
        throw "writer not yet implemented";
    }
  }

  const std::string& Writer::ext( ) const {
    return extension;
  }

  void Writer::compression( int lev ) {
    compress = lev;
  }

  int Writer::compression( ) const {
    return compress;
  }

  int Writer::initDataSet( ) {
    return 0;
  }

  void Writer::stopAfterFirstFile( bool onlyone ) {
    testrun = onlyone;
  }

  FileNamer& Writer::filenamer( ) const {
    return *namer.get( );
  }

  std::vector<std::string> Writer::write( Reader * from, SignalSet * data ) {
    int patientno = 1;

    output( ) << "init data set" << std::endl;
    namer->patientOrdinal( patientno );
    int initrslt = initDataSet( );
    std::vector<std::string> list;
    if ( initrslt < 0 ) {
      std::cerr << "cannot init dataset: " + namer->last( ) << std::endl;
      return list;
    }

    output( ) << "filling data" << std::endl;
    ReadResult retcode = from->fill( data );

    int files = 1;
    while ( retcode != ReadResult::ERROR ) {
      data->complete( );
      drain( data );
      namer->fileOrdinal( files++ );

      if ( 0 != data->metadata( ).count( OffsetTimeSignalSet::COLLECTION_OFFSET ) ) {
        namer->timeoffset_ms( std::stol( data->metadata( ).at( OffsetTimeSignalSet::COLLECTION_OFFSET ) ) );
      }

      if ( testrun ) {
        retcode = ReadResult::END_OF_FILE;
      }


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

        data->reset( true );
        namer->patientOrdinal( patientno );
        output( ) << "init data set" << std::endl;
        initDataSet( );
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

  void Writer::quiet( bool q ) {

    bequiet = q;
  }

  void Writer::filenamer( const FileNamer& p ) {
    namer.reset( new FileNamer( p ) );
  }

  std::ostream& Writer::output( ) const {
    return ( bequiet ? ( std::ostream& ) ss : std::cout );
  }

  int Writer::tz_offset( ) const {
    return gmt_offset;
  }

  const std::string& Writer::tz_name( ) const {
    return timezone;
  }
}