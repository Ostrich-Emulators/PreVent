/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "MatWriter.h"
#include <limits>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <bitset>
#include <iomanip>
#include <vector>

#include "config.h"
#include "SignalSet.h"
#include "SignalData.h"
#include "SignalUtils.h"
#include "FileNamer.h"
namespace FormatConverter{

  MatWriter::MatWriter( MatVersion ver ) : Writer( "mat" ), version( ver ) { }

  MatWriter::MatWriter( const MatWriter& c ) : Writer( "mat" ) { }

  MatWriter::~MatWriter( ) { }

  int MatWriter::initDataSet( ) {
    output( ) << "Warning: the MatWriter may be out of date...please check the output" << std::endl;
    compress = ( 0 == compression( ) ? MAT_COMPRESSION_NONE : MAT_COMPRESSION_ZLIB );

    char fulldatetime[sizeof "Thu Nov 31 10:10:27 1997"];
    time_t now;
    time( &now );
    std::strftime( fulldatetime, sizeof fulldatetime, "%c", gmtime( &now ) );

    std::stringstream header;
    mat_ft ver;
    header << "MATLAB ";
    switch ( version ) {
      case MV5:
        ver = MAT_FT_MAT5;
        header << "5.0";
        break;
      case MV4:
        ver = MAT_FT_MAT4;
        header << "4.0";
        break;
      default:
        ver = MAT_FT_MAT73;
        header << "7.3";
    }

    header << "  MAT-file, Platform: " << osname
        << ", Created by: fmtcnv (rpb6eg@virginia.edu) on: "
        << fulldatetime;

    tempfileloc = filenamer( ).filenameNoExt( );
    matfile = Mat_CreateVer( tempfileloc.c_str( ), header.str( ).c_str( ), ver );

    return !( matfile );
  }

  std::vector<std::string> MatWriter::closeDataSet( ) {
    Mat_Close( matfile );

    // copy our temporary mat file to its final location
    std::ifstream src( tempfileloc, std::ios::binary );
    std::ofstream dst( filenamer( ).last( ), std::ios::binary | std::ios::trunc );

    dst << src.rdbuf( );
    remove( tempfileloc.c_str( ) );

    src.close( );
    dst.close( );

    std::vector<std::string> ret;
    ret.push_back( filenamer( ).last( ) );
    return ret;
  }

  int MatWriter::drain( SignalSet * info ) {
    dataptr = info;
    filenamer( ).filename( info );

    writeVitals( dataptr->vitals( ) );

    if ( !this->skipwaves( ) ) {
      std::map<double, std::vector < SignalData *>> freqgroups;
      for ( auto& ds : info->waves( ) ) {
        freqgroups[ds->hz( )].push_back( ds );
      }

      for ( auto& ds : freqgroups ) {
        writeWaves( ds.first, ds.second );
      }
    }

    return 0;
  }

  int MatWriter::writeStrings( const std::string& label, std::vector<std::string>& strings ) {
    const size_t rows = strings.size( );


    // WARNING: matlab/matio needs column-major ordering

    // this is the logic for writing a character matrix, where every name
    // has the same number of spaces
    //  size_t cols = 0;
    //  for ( auto& s : strings ) {
    //    if ( s.size( ) > cols ) {
    //      cols = s.size( );
    //    }
    //  }
    //
    //  size_t dims[] = { rows, cols };
    //  char strdata[cols][rows] = { };
    //  for ( int c = 0; c < cols; c++ ) {
    //    for ( int r = 0; r < rows; r++ ) {
    //      strdata[c][r] = ( c > strings[r].size( ) ? ' ' : strings[r][c] );
    //    }
    //  }
    //
    //  matvar_t * var = Mat_VarCreate( label.c_str( ), MAT_C_CHAR, MAT_T_UTF8, 2,
    //      dims, strdata, 0 );

    // this is the code for writing cells, which seems more appropriate to me
    size_t dims[] = { 1, rows };
    matvar_t * var = Mat_VarCreate( label.c_str( ), MAT_C_CELL, MAT_T_CELL, 2,
        dims, NULL, 0 );

    for ( size_t i = 0; i < rows; i++ ) {
      size_t strdims[2] = { 1, strings[i].size( ) };
      char * text = (char *) strings[i].c_str( );
      matvar_t * vart = Mat_VarCreate( NULL, MAT_C_CHAR, MAT_T_UTF8, 2, strdims,
          text, 0 );
      Mat_VarSetCell( var, i, vart );
      // does vart get free'd when var does?
    }

    int ok = Mat_VarWrite( matfile, var, compress );
    Mat_VarFree( var );
    return ok;
  }

  int MatWriter::writeVitals( std::vector<SignalData *>signals ) {
    dr_time earliest;
    dr_time latest;

    SignalUtils::firstlast( signals, &earliest, &latest );

    float freq = ( *signals.begin( ) )->hz( );
    const int timestep = ( freq < 1 ? 1 / freq : 1 );

    std::vector<std::string> labels;
    std::vector<std::string> uoms;
    std::map<std::string, int> scales;
    for ( auto& m : signals ) {
      labels.push_back( m->name( ) );
      scales[m->name( )] = m->scale( );
      uoms.push_back( m->uom( ) );
    }

    auto syncd = SignalUtils::syncDatas( signals );
    const int rows = syncd.size( );
    const int cols = signals.size( );

    size_t dims[2] = { (size_t) rows, (size_t) cols };

    int timestamps[rows] = { 0 };
    short vitals[rows * cols];
    int row = 0;
    timestamps[0] = earliest;
    for ( std::vector<int>& rowcols : syncd ) {
      int col = 0;
      for ( auto& rawval : rowcols ) {
        short val = ( scales[labels[col]] > 1
            ? rawval * scales[labels[col]]
            : static_cast<short> ( rawval ) );

        // WARNING: we're transposing these values
        // (because that's how matio wants them)!
        vitals[col * rows + row] = val;
        col++;
      }
      row++;
      timestamps[row] = timestamps[row - 1] + timestep;
    }

    matvar_t * var = Mat_VarCreate( "vitals", MAT_C_INT16, MAT_T_INT16, 2, dims,
        vitals, 0 );

    Mat_VarWrite( matfile, var, compress );
    Mat_VarFree( var );

    // timestamps
    dims[0] = rows;
    dims[1] = 1;

    var = Mat_VarCreate( "vt", MAT_C_INT32, MAT_T_INT32, 2, dims, timestamps, 0 );
    Mat_VarWrite( matfile, var, compress );
    Mat_VarFree( var );

    // scales
    dims[0] = 1;
    dims[1] = cols;
    short scalesarr[cols];
    for ( int i = 0; i < cols; i++ ) {
      scalesarr[i] = scales[labels[i]];
    }
    var = Mat_VarCreate( "vscales", MAT_C_INT16, MAT_T_INT16, 2, dims, scalesarr, 0 );
    Mat_VarWrite( matfile, var, compress );
    Mat_VarFree( var );

    // units of measure
    writeStrings( "vuom", uoms );
    writeStrings( "vlabels", labels );

    // FIXME: (metadata?)

    return 0;
  }

  int MatWriter::writeWaves( double freq, std::vector<SignalData *> oldsignals ) {
    dr_time earliest;
    dr_time latest;

    const std::string sfx = std::to_string( freq ) + "hz";

    // FIXME: need to sync times
    output( ) << "need to sync times first!" << std::endl;
    auto signals = SignalUtils::sync( oldsignals );
    auto signalvec = std::vector<SignalData *>( );
    for ( const auto& s : signals ) {
      signalvec.push_back( s.get( ) );
    }
    SignalUtils::firstlast( signalvec, &earliest, &latest );

    auto alltimes64 = signals[0]->times( );
    std::vector<int> alltimes;
    alltimes.reserve( alltimes64.size( ) );
    for ( dr_time& t64 : alltimes64 ) {
      alltimes.push_back( static_cast<int> ( t64 ) );
    }

    std::vector<std::string> labels;
    std::vector<std::string> uoms;
    for ( auto& m : signals ) {
      labels.push_back( m->name( ) );
      uoms.push_back( m->uom( ) );

      // re-add signals (metadata only) to dataptr

      // FIXME: we need this line!
      //dataptr->waves( )[m->name( )] = m->shallowcopy( true );
    }

    // units of measure
    writeStrings( "wuom" + sfx, uoms );
    writeStrings( "wlabels" + sfx, labels );

    const size_t rows = signals[0]->size( ) * freq;
    const int cols = signals.size( );

    size_t dims[2] = { rows, 1 };

    // each wave gets its own variable; keep track so we can free them later
    std::vector<matvar_t *> vars;
    for ( int i = 0; i < cols; i++ ) {
      matvar_t * var = Mat_VarCreate( signals[i]->name( ).c_str( ),
          MAT_C_INT16, MAT_T_INT16, 2, dims, NULL, 0 );
      Mat_VarWriteInfo( matfile, var );
      vars.push_back( var );
    }

    const size_t datachunksz = freq * 2000; // arbitrary, but on the big side
    std::vector<std::vector<short>> datas( cols );

    int start[2] = { 0, 0 };
    int edge[2] = { (int) datachunksz, 1 };
    int stride[2] = { 1, 1 };
    for ( size_t col = 0; col < signals.size( ); col++ ) {
      std::unique_ptr<SignalData>& signal = signals[col];
      std::vector<short> data;
      data.reserve( datachunksz );

      start[0] = 0;
      edge[0] = { (int) datachunksz };

      while ( !signal->empty( ) ) {
        const auto& datarow = signal->pop( );
        datarow->rescale( signal->scale( ) );

        std::vector<short> slices = datarow->shorts( );
        data.insert( data.end( ), slices.begin( ), slices.end( ) );

        if ( datachunksz == data.size( ) ) {
          Mat_VarWriteData( matfile, vars[col], &data[0], start, stride, edge );
          data.clear( );
          data.reserve( datachunksz );
          start[0] += datachunksz;
        }
      }

      // write any leftover values
      edge[0] = data.size( );
      Mat_VarWriteData( matfile, vars[col], &data[0], start, stride, edge );
      data.clear( );
      Mat_VarFree( vars[col] );
    }


    // timestamps
    // we can write these all in one go, because we're only writing the second for 
    // each hz readings, not the fractional seconds
    start[0] = 0;
    start[1] = 0;
    dims[0] = alltimes.size( );

    matvar_t * var = Mat_VarCreate( std::string( "wt" + sfx ).c_str( ),
        MAT_C_INT32, MAT_T_INT32, 2, dims, &alltimes[0], 0 );
    Mat_VarWrite( matfile, var, compress );
    Mat_VarFree( var );

    // FIXME: (metadata?)

    return 0;
  }
}