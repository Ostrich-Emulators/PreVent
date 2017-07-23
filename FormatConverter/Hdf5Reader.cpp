/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Hdf5Reader.h"
#include "DataRow.h"
#include "SignalData.h"

Hdf5Reader::Hdf5Reader( ) {

}

Hdf5Reader::Hdf5Reader( const Hdf5Reader& ) {

}

Hdf5Reader::~Hdf5Reader( ) {
}

int Hdf5Reader::prepare( const std::string& recordset, ReadInfo& info ) {
  return -1;
}

void Hdf5Reader::finish( ) {
}

ReadResult Hdf5Reader::fill( ReadInfo& info, const ReadResult& ) {
  return ReadResult::ERROR;
}

int Hdf5Reader::getSize( const std::string& input ) const {
  return 0;
}