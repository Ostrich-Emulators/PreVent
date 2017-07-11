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

time_t WfdbReader::convert( const char * timestr ) {
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
      strptime( timestr, "%H:%M:%S", &timeDate );
    }
  }
  else {
    // we have a date and time
    strptime( timestr, "[%H:%M:%S %D]", &timeDate );
  }

  // mktime includes timezone, and we want UTC
  return timegm( &timeDate );
}

int WfdbReader::prepare( const std::string& recordset, ReadInfo& info ) {
  sigcount = isigopen( (char *) ( recordset.c_str( ) ), NULL, 0 );
  if ( sigcount > 0 ) {
    siginfo = new WFDB_Siginfo[sigcount];

    sigcount = isigopen( (char *) ( recordset.c_str( ) ), siginfo, sigcount );

    for ( int i = 0; i < sigcount; i++ ) {
      std::unique_ptr<SignalData>& dataset = info.addVital( siginfo[i].desc );

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

ReadResult WfdbReader::readChunk( ReadInfo& info ) {
  WFDB_Sample v[sigcount];
  int retcode = getvec( v );
  int sampleno = 0;

  while ( retcode > 0 ) {
    for ( int j = 0; j < sigcount; j++ ) {
      char * timer = timstr( sampleno );
      time_t timet = convert( timer );
      // see https://www.physionet.org/physiotools/wpg/strtim.htm#timstr-and-strtim
      // for what timer is

      WFDB_Frequency freqhz = getifreq( );
      if ( freqhz > 1 ) {
        // we have a waveform, so worry about sample numbers

        // WARNING: also, WFDB can specify sample rate, so we can't rely on
        // it being always a multiple of 60
      }

      DataRow row( timet, std::to_string( v[j] ) );

      std::unique_ptr<SignalData>& dataset = info.addVital( siginfo[j].desc );
      dataset->add( row );
    }

    retcode = getvec( v );
    sampleno++;
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

int WfdbReader::getSize( const std::string& input ) const {
  // input is a record name, so we need to figure out how big that data will
  // eventually be
  int nsig = isigopen( (char *) ( input.c_str( ) ), NULL, 0 );
  if ( nsig < 1 ) {
    return -1;
  }

  WFDB_Siginfo * siginfo = new WFDB_Siginfo[nsig];
  nsig = isigopen( (char *) ( input.c_str( ) ), siginfo, nsig );
  long sz = 0;
  for ( int i = 0; i < nsig; i++ ) {
    sz += siginfo[i].nsamp * siginfo[i].fmt;
  }

  delete [] siginfo;

  return (int) sz;
}