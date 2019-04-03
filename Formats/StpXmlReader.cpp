/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CacheFileHdf5Writer.cpp
 * Author: ryan
 * 
 * Created on August 26, 2016, 12:55 PM
 * 
 * Almost all the zlib code was taken from http://www.zlib.net/zlib_how.html
 */

#include "StpXmlReader.h"
#include "SignalData.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <map>
#include <memory>

const std::set<std::string> StpXmlReader::Hz60({ "RR", "VNT_PRES", "VNT_FLOW" } );
const std::set<std::string> StpXmlReader::Hz120({ "ICP1", "ICP2", "ICP4", "LA4" } );
const int StpXmlReader::INDETERMINATE = 0;
const int StpXmlReader::INHEADER = 1;
const int StpXmlReader::INVITAL = 2;
const int StpXmlReader::INWAVE = 4;
const int StpXmlReader::INNAME = 8;


time_t prevtime;
time_t currvstime;
time_t lastvstime;
time_t currwavetime;
time_t lastwavetime;
long currsegidx;
bool warnMissingName;
bool warnJunkData;
bool v8;
bool isphilips;
bool isix;
bool warnedix;

StpXmlReader::StpXmlReader( ) : XmlReaderBase( "STP XML" ), prevtime( 0 ),
currvstime( 0 ), lastvstime( 0 ), currwavetime( 0 ), lastwavetime( 0 ),
recordtime( 0 ), currsegidx( 0 ), warnMissingName( true ), warnJunkData( true ),
v8( false ), isphilips( false ), isix( false ), warnedix( false ),
state( INDETERMINATE ) {
}

StpXmlReader::StpXmlReader( const StpXmlReader& orig ) : XmlReaderBase( orig ) {
}

StpXmlReader::~StpXmlReader( ) {
}

void StpXmlReader::setstate( int st ) {
  state = st;
}

void StpXmlReader::start( const std::string& element, std::map<std::string, std::string>& attributes ) {
  if ( "FileInfo" == element || "DeviceInformation" == element ) {
    setstate( INHEADER );
  }
  else if ( std::string::npos != element.find( "Segment" ) ) {
    currsegidx = std::stol( attributes["Offset"] );
  }
  else if ( "VitalSigns" == element ) {
    setstate( INVITAL );
    lastvstime = currvstime;
    if ( 0 != attributes.count( "CollectionTimeUTC" ) ) {
      currvstime = time( attributes["CollectionTimeUTC"] );
    }
    else {
      currvstime = time( attributes[0 == attributes.count( "CollectionTime" ) ? "Time" : "CollectionTime"], true );
    }

    if ( 0 == filler->offsets( ).count( currsegidx ) ) {
      filler->addOffset( currsegidx, currvstime );
    }

    if ( isRollover( lastvstime, currvstime ) ) {
      setResult( ReadResult::END_OF_DAY );
      prevtime = currvstime;
      startSaving( currvstime );
    }
  }
  else if ( "Waveforms" == element ) {
    setstate( INWAVE );
    lastwavetime = currwavetime;
    if ( 0 != attributes.count( "CollectionTimeUTC" ) ) {
      currwavetime = time( attributes["CollectionTimeUTC"] );
    }
    else {
      currwavetime = time( attributes[0 == attributes.count( "CollectionTime" ) ? "Time" : "CollectionTime"], true );
    }

    if ( 0 == filler->offsets( ).count( currsegidx ) ) {
      filler->addOffset( currsegidx, currwavetime );
    }

    if ( isRollover( lastwavetime, currwavetime ) ) {
      setResult( ReadResult::END_OF_DAY );
      prevtime = currwavetime;
      startSaving( currwavetime );
    }
  }
  else if ( "PatientName" == element ) {
    setstate( INNAME );
  }
  else if ( "Value" == element ) {
    if ( 0 != attributes.count( "UOM" ) ) {
      uom = attributes["UOM"];
    }

    attrs = attributes;
    attrs.erase( "UOM" );

    for ( auto it = attrs.begin( ); it != attrs.end( ); ) {
      if ( it->second.empty( ) ) {
        it = attrs.erase( it );
      }
      else {
        it++;
      }
    }
  }
  else if ( "WaveformData" == element ) {
    attrs = attributes;
    label = attributes[v8 || isphilips ? "Label" : "Channel"];
    uom = attributes["UOM"];
    if ( v8 || isphilips ) {
      v8samplerate = std::stoi( attributes["SampleRate"] );
      if ( isix && !v8 && v8samplerate <= 16 ) {
        // for pre-v8 PhilipsIX inputs, we appear to always get 256 values,
        // but only for signals with SampleRate less than or equal to 16 
        v8samplerate = 256;
        if ( !warnedix ) {
          warnedix = true;
          std::cerr << "Assuming 256 samples/sec from this Philips IX data" << std::endl;
        }
      }
    }
  }

  //  std::cout << "start " << element << std::endl;
  //  if ( !attrs.empty( ) ) {
  //    for ( auto& m : attrs ) {
  //      std::cout << "\t" << m.first << "=>" << m.second << std::endl;
  //    }
  //  }
}

void StpXmlReader::end( const std::string& element, const std::string& text ) {
  if ( INHEADER == state ) {
    if ( "FileInfo" == element ) {
      setstate( INDETERMINATE );
    }
    else if ( "FamilyType" == element || "Family" == element ) {
      filler->setMeta( element, text );
      if ( text.length( ) >= 7 ) {
        isphilips = ( "Philips" == text.substr( 0, 7 ) );
        isix = ( "PhilipsIX" == text );
      }
    }
    else if ( "Size" != element ) {
      filler->setMeta( element, text );
    }
  }
  else if ( INNAME == state ) {
    if ( 0 == prevtime ) {
      filler->setMeta( "Patient Name", text );
    }
    else {

      std::string pname = ( 0 == filler->metadata( ).count( "Patient Name" )
          ? ""
          : filler->metadata( ).at( "Patient Name" ) );
      if ( text != pname ) {
        setResult( ReadResult::END_OF_PATIENT );
        // we've cut over to a new set of data, so
        // save the data we parse now to a different SignalSet
        startSaving( 0 );
        saved.setMeta( "Patient Name", text );
      }
    }
    setstate( INDETERMINATE );
  }
  else if ( INVITAL == state ) {
    if ( "Par" == element || "Parameter" == element ) {
      label = text;
    }
    else if ( "Value" == element ) {
      value = text;
    }
    else if ( "TimeUTC" == element ) {
      recordtime = time( text );
    }
    else if ( "VS" == element || "VitalSign" == element ) {
      bool added = false;

      // skip missing value marker for vitals
      if ( SignalData::MISSING_VALUESTR != value ) {

        // v8 has a lot of stuff in value that isn't really a value
        // so we need to check that we have a number here
        try {
          std::stof( value );
          std::unique_ptr<SignalData>& sig = filler->addVital( label, &added );

          if ( added ) {
            if ( isphilips ) {
              sig->setChunkIntervalAndSampleRate( isix ? 1024 : 1000, 1 );

            }
            else {
              sig->setChunkIntervalAndSampleRate( 2000, 1 );
            }

            for ( auto x : sig->metad( ) ) {
              output( ) << x.first << ":=>" << x.second << std::endl;
            }

            if ( !uom.empty( ) ) {
              sig->setUom( uom );
            }

          }

          DataRow row( currvstime, value, attrs );
          sig->add( row );
          if ( !( 0 == recordtime || recordtime == currvstime ) ) {
            sig->recordEvent( "collection discrepancy", recordtime );
          }
          recordtime = 0;
        }
        catch ( std::invalid_argument ) {
          // don't really care since we're not adding the data to our dataset
          // value = MISSING_VALUESTR;
        }
      }
    }
    else if ( "VitalSigns" == element ) {
      setstate( INDETERMINATE );
    }
  }
  else if ( INWAVE == state ) {
    if ( "WaveformData" == element ) {
      if ( label.empty( ) ) {
        if ( warnMissingName ) {
          std::cerr << "skipping unnamed waveforms" << std::endl;
          warnMissingName = false;
        }
      }
      else {
        std::string wavepoints = text;
        // reverse the oversampling (if any) and set the true Hz
        int thz = 240;
        if ( v8 || isphilips ) {
          thz = v8samplerate;
        }
        else {
          if ( 0 != Hz60.count( label ) ) {
            wavepoints = resample( text, 60 );
            thz = 60;
          }
          else if ( 0 != Hz120.count( label ) ) {
            wavepoints = resample( text, 120 );
            thz = 120;
          }
        }
        const int hz = thz;

        if ( waveIsOk( wavepoints ) ) {
          bool first;
          std::unique_ptr<SignalData>& sig = filler->addWave( label, &first );
          if ( first ) {
            if ( v8 ) {

              int interval = std::stoi( attrs.at( "SamplePeriodInMsec" ) );
              int reads = v8samplerate;
              if ( !isphilips ) {
                interval *= 2;
                reads *= 2;
              }
              sig->setChunkIntervalAndSampleRate( interval, reads );
            }
            else {
              // Stp always reads in 1024ms increments for Philips, 2s for GE monitors
              if ( isphilips ) {
                sig->setChunkIntervalAndSampleRate( isix ? 1024 : 1000, hz );
              }
              else {
                sig->setChunkIntervalAndSampleRate( 2000, 2 * hz );
              }
            }

            if ( !uom.empty( ) ) {
              sig->setUom( uom );
            }

            if ( 0 != attrs.count( "Cal" ) ) {
              sig->setMeta( "Cal", attrs["Cal"] );
            }
          }

          DataRow row( currwavetime, wavepoints, attrs );
          sig->add( row );
        }
        else if ( warnJunkData ) {
          warnJunkData = false;
          std::cerr << "skipping waveforms with no usable data" << std::endl;
        }
      }
    }
    else if ( "WaveformData" == element ) {
      setstate( INDETERMINATE );
    }
  }

  // std::cout << "end " << element << std::endl; //"(" << text << ")" << std::endl;
}

void StpXmlReader::comment( const std::string& text ) {
  size_t pos = text.find( "Version " );
  if ( pos != std::string::npos ) {
    std::string versiontxt = text.substr( pos + 8 );
    int ver = std::stof( versiontxt );
    if ( ver >= 8.0 ) {

      v8 = true;
    }
  }
}

std::string StpXmlReader::resample( const std::string& data, int hz ) {
  /**
  The data arg is always sampled at 240 Hz and is in a 2-second block (480 vals)
  However, the actual wave sampling rate may be less (will always be a multiple of 60Hz)
  so we need to remove the extra values if the values are up-sampled
   **/

  if ( !( 60 == hz || 120 == hz ) ) {
    return data;
  }


  std::vector<std::string> valvec;
  valvec.reserve( hz * 2 );

  std::stringstream stream( data );
  if ( 120 == hz ) {
    // remove every other value
    for ( std::string each; std::getline( stream, each, ',' ); valvec.push_back( each ) ) {
      std::getline( stream, each, ',' );
    }
  }
  else if ( 60 == hz ) {
    // keep one val, skip the next 3
    for ( std::string each; std::getline( stream, each, ',' ); valvec.push_back( each ) ) {
      std::getline( stream, each, ',' );
      std::getline( stream, each, ',' );
      std::getline( stream, each, ',' );
    }
  }

  std::string newvals;
  for ( auto s : valvec ) {
    if ( !newvals.empty( ) ) {

      newvals.append( "," );
    }
    newvals.append( s );
  }

  return newvals;
}

bool StpXmlReader::waveIsOk( const std::string& wavedata ) {
  // if all the values are -32768 or -32753, this isn't a valid reading
  std::stringstream stream( wavedata );
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    if ( !( "-32768" == each || "-32753" == each ) ) {
      return true;
    }
  }
  return false;
}
