
#include "OutputSignalData.h"
#include "SignalUtils.h"
#include "DataRow.h"
#include <cmath>

namespace FormatConverter{

  OutputSignalData::OutputSignalData( std::ostream& out ) : BasicSignalData( "-" ),
      output( out ) { }

  OutputSignalData::~OutputSignalData( ) { }

  bool OutputSignalData::add( std::unique_ptr<DataRow> row ) {
    int period = BasicSignalData::chunkInterval( );
    int mspervalue = period / BasicSignalData::readingsPerChunk( );
    int scale = BasicSignalData::scale( );

    dr_time time = row->time;

    if ( 0 == scale ) {
      for ( auto x : row->ints() ) {
        output << time << " " << x << std::endl;
        time += mspervalue;
      }
    }
    else {
      for ( auto& x : row->doubles() ) {
        output << time << " " << SignalUtils::tosmallstring( x, scale ) << std::endl;
        time += mspervalue;
      }
    }

    return true;
  }
}