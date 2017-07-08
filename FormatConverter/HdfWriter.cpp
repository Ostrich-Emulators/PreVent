/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "HdfWriter.h"
#include <iostream>

#include "FromReader.h"
#include "DataSetDataCache.h"
#include "DataRow.h"

HdfWriter::HdfWriter( ) {
}

void HdfWriter::write( std::unique_ptr<FromReader>& from ) {
  auto& vs = from->vitals( );
  for ( const auto& mapit : vs ) {
    std::cout << mapit.first << std::endl;
    mapit.second->startPopping( );

    int rows = mapit.second->size( );
    for ( int i = 0; i < rows; i++ ) {
      std::unique_ptr<DataRow> row = std::move( mapit.second->pop( ) );
      std::cout << "\t" << row->time << " " << row->data << " "
          << mapit.second->uom( ) << std::endl;
    }
  }
}