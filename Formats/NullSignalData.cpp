
#include "NullSignalData.h"
#include "DataRow.h"
#include <cmath>

namespace FormatConverter{

  NullSignalData::NullSignalData( const std::string& name, bool iswave ) : BasicSignalData( name, iswave ) {
  }

  NullSignalData::~NullSignalData( ) {
  }

  bool NullSignalData::add( const FormatConverter::DataRow& row ) {
    return true;
  }
}