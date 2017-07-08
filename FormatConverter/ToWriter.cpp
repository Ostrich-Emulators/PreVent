
#include "ToWriter.h"

#include "HdfWriter.h"

ToWriter::ToWriter( ) {
}

ToWriter::ToWriter( const ToWriter& ) {

}

ToWriter::~ToWriter( ) {
}

std::unique_ptr<ToWriter> ToWriter::get( const Format& fmt ) {
  switch ( fmt ) {
    case HDF5:
      return std::unique_ptr<ToWriter>( new HdfWriter( ) );
  }

}
