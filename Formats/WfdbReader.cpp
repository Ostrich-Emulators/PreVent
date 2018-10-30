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

WfdbReader::WfdbReader( ) : Reader( "WFDB" ) {
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
  return timegm( &timeDate )* 1000;
}

int WfdbReader::prepare( const std::string& recordset, std::unique_ptr<SignalSet>& info ) {
  int rslt = Reader::prepare( recordset, info );
  if ( 0 != rslt ) {
    return rslt;
  }

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

void WfdbReader::finish( ) {
  delete [] siginfo;
  wfdbquit( );
}

ReadResult WfdbReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& ) {
  WFDB_Sample v[sigcount];
  int retcode = getvec( v );
  int sampleno = 0;

  WFDB_Frequency freqhz = getifreq( );
  int timecount = 0; // how many times have we seen the same time? (we should see it freqhz times, no?)
  dr_time lasttime = -1;
  DataRow currents[sigcount];
  bool iswave = ( freqhz > 1 );

  while ( retcode > 0 ) {
    for ( int j = 0; j < sigcount; j++ ) {
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
                ? info->addWave( siginfo[j].desc )
                : info->addVital( siginfo[j].desc ) );

        if ( !currents[j].data.empty( ) ) {
          // don't add a row on the very first loop through
          dataset->add( currents[j] );
        }

        currents[j].time = timet;
        currents[j].data.clear( );
      }

      if ( !currents[j].data.empty( ) ) {
        currents[j].data.append( "," );
      }
      currents[j].data.append( std::to_string( v[j] ) );
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
