/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "BSIConverter.h"
#include "SignalUtils.h"
#include "Hdf5Writer.h"
#include "BSICache.h"

#include <iostream>
#include <cmath>

namespace FormatConverter{

  std::filesystem::path BSIConverter::convert( const std::filesystem::path& file ) const {
    auto outfile = file;
    outfile.replace_extension( ".hdf5" );

    const auto TEXTCOL = 90;
    auto filestream = std::fstream{ };
    filestream.open( file, std::fstream::in );
    if ( filestream.is_open( ) ) {
      //auto ROWCOUNT = slowlyCountLines( filestream ) - 1;
      filestream.seekg( 0, filestream.beg );

      std::string line;
      std::getline( filestream, line );
      const auto headers = SignalUtils::splitcsv( line );
      auto cachers = std::vector<std::unique_ptr < BSICache >> ( );
      cachers.reserve( headers.size( ) );
      for ( auto h : headers ) {
        if ( '\"' == h[0] ) {
          h = h.substr( 1 );
        }
        if ( '\"' == h[h.length( ) - 1] ) {
          h = h.substr( 0, h.length( ) - 1 );
        }
        cachers.push_back( std::make_unique<BSICache>( h.empty( ) ? "unnamed" : h ) );
      }

      size_t linenum = 0;
      while ( std::getline( filestream, line ) ) {
        linenum++;
        //std::cout << line << std::endl;
        auto lineview = std::string_view{ line };
        if ( '\r' == lineview[lineview.length( ) - 1] ) {
          lineview.remove_suffix( 1 );
        }
        auto row = SignalUtils::splitcsv( lineview );

        for ( size_t col = 0; col < row.size( ); col++ ) {
          if ( TEXTCOL != col ) {
            double val;
            if ( "NA" == row[col] ) {
              val = std::numeric_limits<double>::infinity( );
            }
            else if ( "FALSE" == row[col] ) {
              val = 0;
            }
            else if ( "TRUE" == row[col] ) {
              val = 1;
            }
            else {
              if ( '"' == row[col][0] ) {
                row[col].remove_prefix( 1 );
              }
              if ( '"' == row[col][row[col].length( ) - 1] ) {
                row[col].remove_suffix( 1 );
              }

              try {
                val = std::stod( std::string{ row[col] } );
              }
              catch ( std::exception& x ) {
                std::cerr << "error parsing file at line/col: " << linenum
                    << "/" << col << "; error: " << x.what( ) << std::endl;
                val = std::numeric_limits<double>::infinity( );
              }
            }
            cachers[col]->push_back( val );
          }
          else {
            // the "unit" col (text)
            cachers[col]->push_back( -1 );
          }
        }
      }

      auto pointers = std::vector<BSICache *>( cachers.size( ) );
      for ( size_t i = 0; i < cachers.size( ); i++ ) {
        pointers[i] = cachers[i].get( );
      }
      write( pointers, outfile );
    }

    return outfile;
  }

  size_t BSIConverter::slowlyCountLines( std::fstream& file ) const {
    char * buffer = new char[1024 * 1024];

    size_t newlines = 0;
    while ( !file.eof( ) ) {
      auto starter = file.gcount( );
      file.read( buffer, 1024 * 1024 );
      auto ender = file.gcount( );

      for ( long i = 0; i < ender - starter; i++ ) {
        if ( '\n' == buffer[i] ) {
          newlines++;
        }
      }
    }

    delete [] buffer;
    return newlines;
  }

  void BSIConverter::write( std::vector<BSICache *>& data, const std::filesystem::path& outfile ) const {
    std::cout << "Writing to " << outfile << std::endl;

    H5::Exception::dontPrint( );
    try {
      H5::H5File file( outfile, H5F_ACC_TRUNC );
      auto group = file.createGroup( "imported" );

      for ( auto& column : data ) {
        const hsize_t ROWS = column->size( );
        const auto SLABSIZE = std::min( ROWS, static_cast<hsize_t> ( BSICache::DEFAULT_CACHE_LIMIT ) );

        hsize_t dims[] = { ROWS, 1 };
        H5::DataSpace space( 2, dims );
        H5::DSetCreatPropList props;
        hsize_t chunkdims[] = { 0, 0 };
        Hdf5Writer::autochunk( dims, 2, sizeof ( double ), chunkdims );
        props.setChunk( 2, chunkdims );
        props.setShuffle( );
        props.setDeflate( 6 );

        H5::DataSet ds = group.createDataSet( column->name( ), H5::PredType::IEEE_F64LE, space, props );

        hsize_t offset[] = { 0, 0 };
        hsize_t count[] = { 0, 1 };

        size_t startidx = 0;
        auto buffer = std::vector<double>( );
        while ( startidx < ROWS ) {
          const auto ADDS = std::min( SLABSIZE, ROWS - startidx );
          auto endidx = startidx + ADDS;

          offset[0] += count[0];
          count[0] = ADDS;
          space.selectHyperslab( H5S_SELECT_SET, count, offset );

          H5::DataSpace memspace( 2, count );
          column->fill( buffer, startidx, endidx );

          ds.write( buffer.data( ), H5::PredType::IEEE_F64LE, memspace, space );
          buffer.clear( );
          startidx += ADDS;
        }
      }
    }
    catch ( std::exception& x ) {
      std::cerr << "unable to write to: " << outfile << "; error: " << x.what( ) << std::endl;
    }
  }
}