/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "UmWfdbReader.h"
#include "DataRow.h"
#include "SignalData.h"

#include <ctime>
#include <iostream>
#include <fstream>
#include <iostream>
#include <iterator>

namespace FormatConverter {

  UmWfdbReader::UmWfdbReader( ) : WfdbReader( "UM WFDB" ) {
  }

  UmWfdbReader::~UmWfdbReader( ) {
  }

  int UmWfdbReader::prepare( const std::string& headerfile, std::unique_ptr<SignalSet>& info ) {
    int rslt = WfdbReader::prepare( headerfile, info );
    if ( 0 != rslt ) {
      return rslt;
    }

    std::string recordset( headerfile.substr( 0, headerfile.size( ) - 4 ) );
    // recordset should be a directory containing a .hea file, a .numerics.csv
    // file, and optionally a .clock.txt file
    std::string clockfile( recordset );
    clockfile += ".clock.txt";
    std::ifstream clock( clockfile );
    if ( clock.good( ) ) {
      std::string firstline;
      std::getline( clock, firstline );

      std::string timepart( firstline.substr( 2, 19 ) );
      struct tm timeinfo;
      Reader::strptime2( timepart, "%m/%d/%Y %T %z", &timeinfo );

      int tzoff = std::stoi( firstline.substr( 26, 3 ) );
      timeinfo.tm_gmtoff = tzoff * 3600;
      // FIXME: we still need to handle DST properly
      if ( timeinfo.tm_mon > 2 && timeinfo.tm_mon < 10 ) {
        timeinfo.tm_isdst = true;
      }
      time_t tt = std::mktime( &timeinfo );


      setBaseTime( tt * 1000 );
    }

    // read the vitals out of the CSV, too
    return rslt;
  }

  ReadResult UmWfdbReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {
    WFDB_Sample v[sigcount];
    bool iswave = ( freqhz > 1 );

    // see https://www.physionet.org/physiotools/wpg/strtim.htm#timstr-and-strtim
    // for what timer is
    char * timer = timstr( 0 );
    dr_time timet = convert( timer );

    output( ) << "timer: " << timer << std::endl;

    int retcode = 0;
    ReadResult rslt = ReadResult::NORMAL;
    while ( true ) {
      std::map<int, std::vector<int>> currents;
      for ( int i = 0; i < sigcount; i++ ) {
        currents[i].reserve( freqhz );
      }

      for ( size_t i = 0; i < freqhz; i++ ) {
        retcode = getvec( v );
        if ( retcode < 0 ) {
          if ( -3 == retcode ) {
            std::cerr << "unexpected end of file" << std::endl;
            return ReadResult::ERROR;
          }
          else if ( -4 == retcode ) {
            std::cerr << "invalid checksum" << std::endl;
            return ReadResult::ERROR;
          }

          if ( -1 == retcode ) {
            rslt = ReadResult::END_OF_FILE;
          }
        }
        else {
          for ( int signalidx = 0; signalidx < sigcount; signalidx++ ) {
            currents[signalidx].push_back( v[signalidx] );
          }
        }
      }

      for ( int signalidx = 0; signalidx < sigcount; signalidx++ ) {
        if ( !currents[signalidx].empty( ) ) {
          std::unique_ptr<SignalData>& dataset = ( iswave
                  ? info->addWave( siginfo[signalidx].desc )
                  : info->addVital( siginfo[signalidx].desc ) );

          if ( currents[signalidx].size( ) < freqhz ) {
            output( ) << "filling in " << ( freqhz - currents[signalidx].size( ) )
                    << " values for wave " << siginfo[signalidx].desc << std::endl;
            currents[signalidx].resize( freqhz, SignalData::MISSING_VALUE );
          }

          std::ostringstream ss;
          std::copy( currents[signalidx].begin( ), currents[signalidx].end( ) - 1,
                  std::ostream_iterator<int>( ss, "," ) );
          ss << currents[signalidx].back( );

          std::string vals = ss.str( );
          dataset->add( DataRow( timet, vals ) );
        }
      }

      timet += interval;

      if ( ReadResult::END_OF_FILE == rslt ) {
        break;
      }
    }

    return rslt;
  }
}