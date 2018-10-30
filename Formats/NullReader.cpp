/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "NullReader.h"

#include "DataRow.h"
#include "SignalData.h"

NullReader::NullReader( const std::string& name ) : Reader( name ) {

}

NullReader::~NullReader( ) {
}

int NullReader::prepare( const std::string& filename, std::unique_ptr<SignalSet>& info ) {
  return 0;
}

ReadResult NullReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& ) {
  return ReadResult::END_OF_FILE;
}
