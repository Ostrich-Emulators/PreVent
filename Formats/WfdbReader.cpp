/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WfdbReader.h"
#include "DataRow.h"
#include "SignalData.h"
#include "config.h"

#include <iostream>
#include <iterator>
#include <sys/stat.h>
#include <math.h>
#include <libgen.h>
#include <cstring>
#include <unistd.h>

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif

namespace FormatConverter{

  WfdbReader::WfdbReader( ) : Reader( "WFDB" ) { }

  WfdbReader::WfdbReader( const std::string& name ) : Reader( name ), extra_ms( 0 ),
      basetimeset( false ), framecount( 0 ) { }

  WfdbReader::~WfdbReader( ) { }

  void WfdbReader::setBaseTime( const dr_time& basetime ) {
    _basetime = basetime;
    dr_time modded = modtime( basetime );

    char * timepart = new char[40];
    char * datepart = new char[40];
    time_t raw = ( modded / 1000 );
    struct tm * timeinfo = localtime( &raw );

    // NOTE: WFDB's setbasetime does NOT seem to handle ms, so we skip it, too
    extra_ms = ( modded % 1000 );
    strftime( timepart, 80, "%H:%M:%S", timeinfo );
    strftime( datepart, 80, " %d/%m/%Y", timeinfo );

    std::string timestr( timepart );
    //timestr += std::to_string( ms );
    timestr.append( datepart );

    setbasetime( (char *) timestr.c_str( ) );
    delete [] timepart;
    delete [] datepart;

    basetimeset = true;
  }

  dr_time WfdbReader::convert( const char * mstimestr ) {
    // HH:MM:SS format by timstr, with leading zero digits and colons suppressed.
    // If t is zero or negative, it is taken to represent negated elapsed time from
    // the beginning of the record, and it is converted to a time of day using the base
    // time for the record as indicated by the ‘hea’ file; in this case, if the
    // base time is defined, the string will contain all digits even if there are
    // leading zeroes, it will include the date if a base date is defined, and it
    // will be marked as a time of day by being bracketed (e.g., ‘[08:45:00.387 23/04/1989]’).
    std::string checker( mstimestr );
    int ms = 0;

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
        strptime2( mstimestr, "%H:%M:%S", &timeDate );
      }
    }
    else {
      // we have a date (and time?)
      strptime2( mstimestr, "[%H:%M:%S", &timeDate );
      char substr[10];
      memcpy( substr, &mstimestr[10], 3 );
      ms = std::stoi( substr );

      memcpy( substr, &mstimestr[14], 10 );

      strptime2( substr, "%d/%m/%Y", &timeDate );
    }

    // mktime includes timezone, and we want UTC
    return modtime( timegm( &timeDate ) * 1000 + ms );
  }

  dr_time WfdbReader::basetime( ) const {
    return _basetime;
  }

  int WfdbReader::base_ms( ) const {
    return extra_ms;
  }

  int WfdbReader::prepare( const std::string& headername, std::unique_ptr<SignalSet>& info ) {
    int rslt = Reader::prepare( headername, info );
    if ( 0 != rslt ) {
      return rslt;
    }

    std::string path( ". " );
    char header_c[headername.size( ) + 1];
    strncpy( header_c, headername.c_str( ), headername.size( ) + 1 );
    std::string wfdbdir( dirname( header_c ) );
    path += wfdbdir;

#ifdef __CYGWIN__
    size_t size = cygwin_conv_path( CCP_WIN_A_TO_POSIX | CCP_RELATIVE, wfdbdir.c_str( ), NULL, 0 );
    if ( size < 0 ) {
      std::cerr << "cannot resolve path: " << path << std::endl;
      return -1;
    }

    std::cout << "size returned is: " << size << std::endl;
    char * cygpath = (char *) malloc( size );
    if ( cygwin_conv_path( CCP_WIN_A_TO_POSIX | CCP_RELATIVE, wfdbdir.c_str( ),
        cygpath, size ) ) {
      perror( "cygwin_conv_path" );
      return -1;
    }

    path.clear( );
    path.append( ". " ).append( cygpath );
    free( cygpath );

#endif
    setwfdb( (char *) path.c_str( ) );

    // the record name is just the basename of the file
    size_t lastslash = headername.rfind( dirsep );
    std::string cutup( headername );
    if ( std::string::npos != lastslash ) {
      cutup = cutup.substr( lastslash + 1 );
    }

    sigcount = isigopen( (char *) cutup.c_str( ), NULL, 0 );
    if ( sigcount > 0 ) {
      WFDB_Frequency wffreqhz = getifreq( );
      bool iswave = ( wffreqhz > 1 );
      siginfo = new WFDB_Siginfo[sigcount];
      framecount = 0;

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

      sigcount = isigopen( (char *) ( cutup.c_str( ) ), siginfo, sigcount );

      for ( int signalidx = 0; signalidx < sigcount; signalidx++ ) {
        std::unique_ptr<SignalData>& dataset = ( iswave
            ? info->addWave( siginfo[signalidx].desc )
            : info->addVital( siginfo[signalidx].desc ) );

        dataset->setChunkIntervalAndSampleRate( interval, freqhz * siginfo[signalidx].spf );
        if ( 1024 == interval ) {
          dataset->setMeta( "Notes", "The frequency from the input file was multiplied by 1.024" );
        }

        if ( NULL != siginfo[signalidx].units ) {
          dataset->setUom( siginfo[signalidx].units );
        }

        dataset->setMeta( "wfdb-gain", siginfo[signalidx].gain );
        dataset->setMeta( "wfdb-spf", siginfo[signalidx].spf );
        dataset->setMeta( "wfdb-adcres", siginfo[signalidx].adcres );
        framecount += siginfo[signalidx].spf;
      }
    }

    if ( 0 == sigcount ) {
      std::cerr << "could not open/read .hea file: " << headername << std::endl;
      return -1;
    }
    return 0;
  }

  void WfdbReader::finish( ) {
    delete [] siginfo;
    wfdbquit( );
  }

  ReadResult WfdbReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {
    WFDB_Sample v[framecount];
    bool iswave = ( freqhz > 1 );

    if ( ReadResult::FIRST_READ == lastrr ) {
      // see https://www.physionet.org/physiotools/wpg/strtim.htm#timstr-and-strtim
      // for what timer is
      curtime = ( basetimeset
          ? basetime( )
          : convert( mstimstr( 0 ) ) );
    }

    int retcode = 0;
    ReadResult rslt = ReadResult::NORMAL;

    size_t framesignal[framecount];
    int sigidx = 0;
    int sigcounter = 0;
    for ( size_t f = 0; f < framecount; f++ ) {
      framesignal[f] = sigidx;
      sigcounter++;
      if ( sigcounter >= siginfo[sigidx].spf ) {
        sigcounter = 0;
        sigidx++;
      }
    }

    while ( true ) {
      std::map<int, std::vector<int>> currents;
      for ( int i = 0; i < sigcount; i++ ) {
        currents[i].reserve( freqhz * siginfo[sigcount].spf );
      }

      for ( size_t i = 0; i < freqhz; i++ ) {
        retcode = getframe( v );
        if ( retcode < 0 ) {
          if ( -3 == retcode ) {
            std::cerr << "unexpected end of file" << std::endl;
            return ReadResult::ERROR;
          }
          else if ( -4 == retcode ) {
            std::cerr << "invalid checksums discovered...continuing, but using missing data value" << std::endl;
            for ( int signalidx = 0; signalidx < sigcount; signalidx++ ) {
              for ( int j = 0; j < siginfo[signalidx].spf; j++ ) {
                currents[signalidx].push_back( SignalData::MISSING_VALUE );
              }
            }
            // return ReadResult::END_OF_FILE;
          }
          else if ( -1 == retcode ) {
            rslt = ReadResult::END_OF_FILE;
            break;
          }
        }
        else {
          for ( size_t j = 0; j < framecount; j++ ) {
            currents[framesignal[j]].push_back( v[j] );
          }
        }
      }

      for ( int signalidx = 0; signalidx < sigcount; signalidx++ ) {
        size_t expected = freqhz * siginfo[signalidx].spf;
        if ( !currents[signalidx].empty( ) ) {
          bool added = false;
          std::unique_ptr<SignalData>& dataset = ( iswave
              ? info->addWave( siginfo[signalidx].desc, &added )
              : info->addVital( siginfo[signalidx].desc, &added ) );

          if ( added ) {
            dataset->setChunkIntervalAndSampleRate( interval, expected );
            if ( 1024 == interval ) {
              dataset->setMeta( "Notes", "The frequency from the input file was multiplied by 1.024" );
            }

            if ( NULL != siginfo[signalidx].units ) {
              dataset->setUom( siginfo[signalidx].units );
            }
          }

          if ( currents[signalidx].size( ) < expected ) {
            output( ) << "filling in " << ( expected - currents[signalidx].size( ) )
                << " values for wave " << siginfo[signalidx].desc << std::endl;
            currents[signalidx].resize( expected, SignalData::MISSING_VALUE );
          }

          dataset->add( std::make_unique<DataRow>( curtime, currents[signalidx] ) );
        }
      }

      dr_time oldtime = curtime;

      curtime += interval;
      if ( isRollover( oldtime, curtime ) ) {
        rslt = ReadResult::END_OF_DAY;
        break;
      }
      else if ( ReadResult::END_OF_FILE == rslt ) {
        break;
      }
    }

    return rslt;
  }
}