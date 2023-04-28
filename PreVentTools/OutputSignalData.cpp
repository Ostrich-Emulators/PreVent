
#include "OutputSignalData.h"
#include "SignalUtils.h"
#include "DataRow.h"
#include <cmath>

namespace FormatConverter{

  OutputSignalData::OutputSignalData( std::ostream& out, bool interpret ) : BasicSignalData( "-" ),
      output( out ), dointerpret( interpret ), first( true ), baseline( 0 ), gain( 1 ) { }

  OutputSignalData::~OutputSignalData( ) { }

  bool OutputSignalData::add( std::unique_ptr<DataRow> row ) {
    int period = BasicSignalData::chunkInterval( );
    int mspervalue = period / BasicSignalData::readingsPerChunk( );
    int scale = BasicSignalData::scale( );

    dr_time time = row->time;

    if ( dointerpret && first ) {
      first = false;
      auto imetas = this->metai( );
      if ( imetas.count( "wfdb-baseline" ) > 0 ) {
        // we have a wfdb file, so we can interpret the gain and baseline values
        auto dmetas = this->metad( );
        if ( dmetas.count( "wfdb-gain" ) > 0 ) {
          this->baseline = imetas.at( "wfdb-baseline" );
          this->gain = dmetas.at( "wfdb-gain" );
        }
      }
    }

    if ( 0 == scale ) {
      for ( auto x : row->ints( ) ) {
        if ( x == SignalData::MISSING_VALUE ) {
          output << time << " " << ( dointerpret ? "-" : SignalData::MISSING_VALUESTR ) << std::endl;
        }
        else {
          output << time << " " << ( dointerpret ? ( x - baseline ) / gain : x ) << std::endl;
        }
        time += mspervalue;
      }
    }
    else {
      for ( auto& x : row->doubles( ) ) {
        if ( x == SignalData::MISSING_VALUE ) {
          output << time << " " << ( dointerpret ? "-" : SignalUtils::tosmallstring( x, scale ) )
              << std::endl;
        }
        else {
          output << time << " " << SignalUtils::tosmallstring( ( dointerpret ? ( x - baseline ) / gain : x ),
              scale ) << std::endl;
        }
        time += mspervalue;
      }
    }

    return true;
  }
}