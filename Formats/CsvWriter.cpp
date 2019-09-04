/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "CsvWriter.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <limits>
#include <fstream>
#include <cmath>
#include <set>

#include "SignalSet.h"
#include "SignalData.h"
#include "SignalUtils.h"
#include "FileNamer.h"

namespace FormatConverter {

  CsvWriter::CsvWriter( ) : Writer( "csv" ) {
  }

  CsvWriter::CsvWriter( const CsvWriter& ) : Writer( "csv" ) {
  }

  CsvWriter::~CsvWriter( ) {
  }

  std::vector<std::string> CsvWriter::closeDataSet( ) {
    std::vector<std::string> vec;
    vec.push_back( filenamer( ).last( ) );
    return vec;
  }

  int CsvWriter::drain( std::unique_ptr<SignalSet>& info ) {
    std::string filename = filenamer( ).filename( info );
    output( ) << "CsvWriter::drain() must be refactored dramatically" << std::endl;
    //  std::ofstream out( filename );
    //  for ( auto& v : info ) {
    //    const int scale = v.get( )->scale( );
    //
    //    if ( v->iswave( ) ) {
    //      out << "wave," << w.get( )->name( ) << std::endl;
    //      out << "time,value,slice" << std::endl;
    //      while ( !w.get( )->empty( ) ) {
    //        const auto& r = w.get( )->pop( );
    //        std::vector<int> vals = r->ints( w.get( )->scale( ) );
    //        for ( int slice = 0; slice < vals.size( ); slice++ ) {
    //          out << r->time << "," << vals[slice] << "," << slice << std::endl;
    //        }
    //      }
    //    }
    //    else {
    //      out << "vital," << v.get( )->name( ) << std::endl;
    //      out << "time,value" << std::endl;
    //      while ( !v.get( )->empty( ) ) {
    //        const auto& r = v.get( )->pop( );
    //        if ( 0 == scale ) {
    //          out << r->time << "," << r->data << std::endl;
    //        }
    //        else {
    //          float data = std::stof( r->data ) * std::pow( 10, scale );
    //          out << r->time << "," << data << std::endl;
    //        }
    //      }
    //    }
    //
    //    out.close( );
    return 0;
  }
}