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
#include <set>

#include "SignalSet.h"
#include "SignalData.h"
#include "SignalUtils.h"
#include "FileNamer.h"

CsvWriter::CsvWriter( ) : Writer( "csv" ) {
}

CsvWriter::CsvWriter( const CsvWriter& ) : Writer( "csv" ){
}

CsvWriter::~CsvWriter( ) {
}

std::vector<std::string> CsvWriter::closeDataSet( ) {
  std::vector<std::string> vec;
  vec.push_back( filenamer().last() );
  return vec;
}

int CsvWriter::drain( SignalSet& info ) {
  std::string filename = filenamer().filename( info );

  std::ofstream out( filename );
  for ( auto& v : info.vitals( ) ) {
    const int scale = v.second->scale( );
    out << "vital," << v.first << std::endl;
    out << "time,value,high limit,low limit" << std::endl;
    while ( !v.second->empty( ) ) {
      const auto& r = v.second->pop( );
      if ( 1 == scale ) {
        out << r->time << "," << r->data << "," << r->high << "," << r->low << std::endl;
      }
      else {
        float data = std::stof( r->data ) / scale;
        float hi = std::stof( r->high ) / scale;
        float lo = std::stof( r->low ) / scale;
        out << r->time << "," << data << "," << hi << "," << lo << std::endl;
      }
    }
  }

  for ( auto& w : info.waves( ) ) {
    out << "wave," << w.first << std::endl;
    out << "time,value,slice" << std::endl;
    while ( !w.second->empty( ) ) {
      const auto& r = w.second->pop( );
      std::vector<int> vals = r->ints( w.second->scale() );
      for ( int slice = 0; slice < vals.size( ); slice++ ) {
        out << r->time << "," << vals[slice] << "," << slice << std::endl;
      }
    }
  }

  out.close( );
  return 0;
}
