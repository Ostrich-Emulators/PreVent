/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WfdbReader.h"
#include "DataRow.h"
#include "SignalData.h"

#include <wfdb/wfdb.h>
#include <iostream>
#include <ctime>
#include <sys/stat.h>
#include <math.h>

namespace FormatConverter {

  WfdbReader::WfdbReader( ) : Reader( "WFDB" ) {
  }

  WfdbReader::WfdbReader( const std::string& name ) : Reader( name ) {
  }

  WfdbReader::~WfdbReader( ) {
  }

  dr_time WfdbReader::convert( const char * timestr ) {
    // HH:MM:SS format by timstr, with leading zero digits and colons suppressed.
    // If t is zero or negative, it is taken to represent negated elapsed time from
    // the beginning of the record, and it is converted to a time of day using the base
    // time for the record as indicated by the ‘hea’ file; in this case, if the
    // base time is defined, the string will contain all digits even if there are
    // leading zeroes, it will include the date if a base date is defined, and it
    // will be marked as a time of day by being bracketed (e.g., ‘[08:45:00 23/04/1989]’).
    std::string checker( timestr );

    struct tm timeDate = { 0 };
    timeDate.tm_year = 70;
    timeDate.tm_mon = 0;
    timeDate.tm_mday = 1;
    timeDate.tm_yday = 0;

    if ( std::string::npos == checker.find( "[" ) ) {
      int firstpos = checker.find( ":" );
      int lastpos = checker.rfind( ":" );

      if ( firstpos == lastpos ) {
        // no hour element
        timeDate.tm_min = atoi( checker.substr( firstpos - 2, 2 ).c_str( ) );
        timeDate.tm_sec = atoi( checker.substr( firstpos + 1, 2 ).c_str( ) );
      }
      else {
        // hour, minute, second
        strptime2( timestr, "%H:%M:%S", &timeDate );
      }
    }
    else {
      // we have a date and time
      strptime2( timestr, "[%H:%M:%S %D]", &timeDate );
    }

    // mktime includes timezone, and we want UTC
    return modtime( timegm( &timeDate )* 1000 );
  }

  int WfdbReader::prepare( const std::string& recordset, std::unique_ptr<SignalSet>& info ) {
    int rslt = Reader::prepare( recordset, info );
    if ( 0 != rslt ) {
      return rslt;
    }

    sigcount = isigopen( (char *) ( recordset.c_str( ) ), NULL, 0 );
    if ( sigcount > 0 ) {
      WFDB_Frequency wffreqhz = getifreq( );
      bool iswave = ( wffreqhz > 1 );
      siginfo = new WFDB_Siginfo[sigcount];

      double intpart;
      double fraction = std::modf( wffreqhz, &intpart );
      if ( 0 == fraction ) {
        interval = 1000;
        freqhz = wffreqhz;
      }
      else {
        interval = 1024;
        freqhz = wffreqhz * 1.024;
        output( ) << "warning: Signals are assumed to be sampled at 1024ms intervals, not 1000ms" << std::endl;
      }

      sigcount = isigopen( (char *) ( recordset.c_str( ) ), siginfo, sigcount );

      for ( int signalidx = 0; signalidx < sigcount; signalidx++ ) {
        std::unique_ptr<SignalData>& dataset = ( iswave
                ? info->addWave( siginfo[signalidx].desc )
                : info->addVital( siginfo[signalidx].desc ) );

        dataset->setChunkIntervalAndSampleRate( interval, freqhz );
        if ( 1024 == interval ) {
          dataset->setMeta( "Notes", "The frequency from the input file was multiplied by 1.024" );
        }

        if ( NULL != siginfo[signalidx].units ) {
          dataset->setUom( siginfo[signalidx].units );
        }
      }
    }

    return ( sigcount > 0 ? 0 : -1 );
  }

  void WfdbReader::finish( ) {
    delete [] siginfo;
    wfdbquit( );
  }

  ReadResult WfdbReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& ) {
    WFDB_Sample v[sigcount];
    int retcode = getvec( v );
    int sampleno = 0;

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