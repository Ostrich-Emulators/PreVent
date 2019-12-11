/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "UmWfdbReader.h"
#include "DataRow.h"
#include "SignalData.h"

#include <wfdb/wfdb.h>
#include <iostream>
#include <ctime>
#include <sys/stat.h>

namespace FormatConverter {

  UmWfdbReader::UmWfdbReader( ) : WfdbReader( "UM WFDB" ) {
  }

  UmWfdbReader::~UmWfdbReader( ) {
  }

  int UmWfdbReader::prepare( const std::string& recordset, std::unique_ptr<SignalSet>& info ) {
    int rslt = Reader::prepare( recordset, info );
    if ( 0 != rslt ) {
      return rslt;
    }

    // recordset should be a directory containing a .hea file, and a
    // .numerics.csv file 


    sigcount = isigopen( (char *) ( recordset.c_str( ) ), NULL, 0 );
    if ( sigcount > 0 ) {
      WFDB_Frequency freqhz = getifreq( );
      bool iswave = ( freqhz > 1 );
      siginfo = new WFDB_Siginfo[sigcount];

      sigcount = isigopen( (char *) ( recordset.c_str( ) ), siginfo, sigcount );

      for ( int i = 0; i < sigcount; i++ ) {
        std::unique_ptr<SignalData>& dataset = ( iswave
                ? info->addWave( siginfo[i].desc )
                : info->addVital( siginfo[i].desc ) );

        dataset->setChunkIntervalAndSampleRate( 1000, freqhz );

        if ( NULL != siginfo[i].units ) {
          dataset->setUom( siginfo[i].units );
        }
      }
    }

    return ( sigcount > 0 ? 0 : -1 );
  }

  void UmWfdbReader::finish( ) {
    delete [] siginfo;
    wfdbquit( );
  }

  ReadResult UmWfdbReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& ) {
    WFDB_Sample v[sigcount];
    int retcode = getvec( v );
    int sampleno = 0;

    WFDB_Frequency freqhz = getifreq( );
    int timecount = 0; // how many times have we seen the same time? (we should see it freqhz times, no?)
    dr_time lasttime = -1;
    FormatConverter::DataRow currents[sigcount];
    bool iswave = ( freqhz > 1 );

    while ( retcode > 0 ) {
      for ( int signalidx = 0; signalidx < sigcount; signalidx++ ) {
        char * timer = timstr( sampleno );
        dr_time timet = convert( timer );
        // see https://www.physionet.org/physiotools/wpg/strtim.htm#timstr-and-strtim
        // for what timer is

        if ( timet == lasttime ) {
          timecount++;
        }
        else {
          timecount = 0;
          lasttime = timet;

          std::unique_ptr<SignalData>& dataset = ( iswave
                  ? info->addWave( siginfo[signalidx].desc )
                  : info->addVital( siginfo[signalidx].desc ) );

          if ( !currents[signalidx].data.empty( ) ) {
            // don't add a row on the very first loop through
            dataset->add( currents[signalidx] );
          }

          currents[signalidx].time = timet;
          currents[signalidx].data.clear( );
        }

        if ( !currents[signalidx].data.empty( ) ) {
          currents[signalidx].data.append( "," );
        }
        currents[signalidx].data.append( std::to_string( v[signalidx] ) );
        // FIXME: we need to string together freqhz values into a string for timet
      }

      retcode = getvec( v );
      sampleno++;
    }

    // now add our last data point
    for ( int j = 0; j < sigcount; j++ ) {
      std::unique_ptr<SignalData>& dataset = ( iswave
              ? info->addWave( siginfo[j].desc )
              : info->addVital( siginfo[j].desc ) );

      currents[j].time = lasttime;
      dataset->add( currents[j] );
    }


    if ( -3 == retcode ) {
      std::cerr << "unexpected end of file" << std::endl;
    }
    else if ( -4 == retcode ) {
      std::cerr << "invalid checksum" << std::endl;
    }

    if ( -1 == retcode ) {
      return ReadResult::END_OF_FILE;
    }

    return ( 0 <= retcode ? ReadResult::NORMAL : ReadResult::ERROR );
  }
}