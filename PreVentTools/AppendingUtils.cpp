/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "AppendingUtils.h"

#include <iostream>
#include <fstream>
#include <vector>

#include "SignalUtils.h"
#include "BasicSignalData.h"
#include "DataRow.h"
#include "Hdf5Writer.h"

namespace FormatConverter {

  const std::string AppendingUtils::DATAGROUP = "/CalculatedData";

  AppendingUtils::AppendingUtils( const std::string& targetfile ) : filename( targetfile ) {
  }

  AppendingUtils::~AppendingUtils( ) {
    if ( file.isValid( file.getId( ) ) ) {
      file.close( );
    }
  }

  int AppendingUtils::append( const std::string& datafile ) {
    if ( !file.isValid( file.getId( ) ) ) {
      H5::Exception::dontPrint( );
      try {
        file = H5::H5File( filename, H5F_ACC_RDWR );
      }
      catch ( H5::FileIException& error ) {
        std::cerr << error.getDetailMsg( ) << std::endl;
        return -1;
      }
      // catch failure caused by the DataSet operations
      catch ( H5::DataSetIException& error ) {
        std::cerr << error.getDetailMsg( ) << std::endl;
        return -2;
      }
    }

    try {
      ensureCustomDataGroupExists( );
      std::unique_ptr<SignalData> signal = parseDataFile( datafile );

      if ( signal ) {
        writeSignal( signal );
      }
      else {
        std::cerr << "No data to append" << std::endl;
        return 1;
      }
    }
    catch ( H5::FileIException& error ) {
      std::cerr << error.getDetailMsg( ) << std::endl;
      return -3;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      std::cerr << error.getDetailMsg( ) << std::endl;
      return -4;
    }

    return 0;
  }

  void AppendingUtils::ensureCustomDataGroupExists( ) {
    if ( !file.exists( DATAGROUP ) ) {
      file.createGroup( DATAGROUP );
    }
  }

  std::unique_ptr<SignalData> AppendingUtils::parseDataFile( const std::string& datafile ) {
    std::ifstream file( datafile );
    std::string line;
    std::string key;
    std::string key2;
    std::string val;
    bool readingdata = false;

    std::unique_ptr<SignalData> signal;
    while ( std::getline( file, line ) ) {
      SignalUtils::trim( line );

      if ( signal ) {
        if ( readingdata ) {
          signal->add( DataRow( 0, line ) );
        }
        else {
          // not reading data yet, so we're setting
          // metadata (or entering data section)

          split( line, key, val );
          if ( "DATA" == key ) {
            readingdata = true;
          }
          else {
            std::string valtype = "s";
            split( key, key2, valtype, ":" );

            if ( "i" == valtype ) {
              signal->setMeta( key2, std::stoi( val ) );
            }
            else if ( "d" == valtype ) {
              signal->setMeta( key2, std::stod( val ) );
            }
            else {
              signal->setMeta( key2, val );
            }
          }
        }
      }
      else {
        // first line has to be NAME, which we'll use to create our SignalData
        split( line, key, val );
        if ( "NAME" != key ) {
          return std::move( signal );
        }

        signal.reset( new BasicSignalData( val ) );
        signal->erasei( SignalData::CHUNK_INTERVAL_MS );
        signal->erasei( SignalData::READINGS_PER_CHUNK );
        signal->erasei( SignalData::MSM );
        signal->erases( SignalData::TIMEZONE );
        signal->erases( "Note on Scale" );
        signal->erases( "Unit of Measure" );
      }
    }

    return std::move( signal );
  }

  int AppendingUtils::writeSignal( std::unique_ptr<SignalData>& data ) {
    std::string dataset = DATAGROUP + "/" + Hdf5Writer::getDatasetName( data );
    H5::DataSet ds;

    size_t rows = data->size( );
    hsize_t sz = rows;
    hsize_t dims[] = { sz, 1 };
    H5::DataSpace space( 2, dims );
    H5::DataSpace memspace( 2, dims );

    if ( !file.exists( dataset ) ) {
      H5::DSetCreatPropList props;
      hsize_t chunkdims[] = { 0, 0 };
      Hdf5Writer::autochunk( dims, 2, sizeof ( int ), chunkdims );
      props.setChunk( 2, chunkdims );
      props.setShuffle( );
      props.setDeflate( 6 );

      ds = file.createDataSet( dataset, H5::PredType::STD_I32LE, space, props );
    }
    else {
      ds = file.openDataSet( dataset );
    }

    Hdf5Writer::writeAttributes( ds, data );
    int scale = data->scale( );
    std::vector<int> datavec;
    datavec.reserve( rows );
    for ( size_t row = 0; row < rows; row++ ) {
      datavec.push_back( data->pop( )->ints( scale )[0] );
    }

    ds.write( &datavec[0], H5::PredType::STD_I32LE, memspace, space );

    return 0;
  }

  void AppendingUtils::split( const std::string& line, std::string& key,
      std::string& val, const std::string& delim ) {
    size_t pos = line.find( delim );
    if ( std::string::npos == pos ) {
      key = line;
    }
    else {
      key = line.substr( 0, pos );
      val = line.substr( pos + 1 );
      SignalUtils::trim( val );
    }

    SignalUtils::trim( key );
  }
}