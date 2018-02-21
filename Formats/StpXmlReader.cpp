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

StpXmlReader::StpXmlReader( ) : XmlReaderBase( "STP XML" ), warnMissingName( true ),
warnJunkData( true ), prevtime( 0 ), currvstime( 0 ), lastvstime( 0 ),
currwavetime( 0 ), lastwavetime( 0 ), state( INDETERMINATE ), currsegidx( 0 ),
v8( false ), isphilips( false ) {
}

StpXmlReader::StpXmlReader( const StpXmlReader& orig ) : XmlReaderBase( orig ) {
}

StpXmlReader::~StpXmlReader( ) {
}

void StpXmlReader::setstate( int st ) {
  state = st;
}

void StpXmlReader::start( const std::string& element, std::map<std::string, std::string>& attributes ) {
  if ( "FileInfo" == element ) {
    setstate( INHEADER );
  }
  else if ( std::string::npos != element.find( "Segment" ) ) {
    currsegidx = std::stol( attributes["Offset"] );
  }
  else if ( "VitalSigns" == element ) {
    setstate( INVITAL );
    lastvstime = currvstime;
    currvstime = time( attributes[ v8 ? "CollectionTimeUTC" : "Time"] );
    if ( anonymizing( ) && isFirstRead( ) ) {
      setDateModifier( currvstime );
    }

    if ( 0 == filler->offsets( ).count( currsegidx ) ) {
      filler->addOffset( currsegidx, datemod( currvstime ) );
    }

    if ( isRollover( lastvstime, currvstime ) ) {
      setResult( ReadResult::END_OF_DAY );
      prevtime = currvstime;
      startSaving( );
    }
  }
  else if ( "Waveforms" == element ) {
    setstate( INWAVE );
    lastwavetime = currwavetime;
    currwavetime = time( attributes[ v8 ? "CollectionTimeUTC" : "Time"] );
    if ( anonymizing( ) && isFirstRead( ) ) {
      setDateModifier( currvstime );
    }

    if ( 0 == filler->offsets( ).count( currsegidx ) ) {
      filler->addOffset( currsegidx, datemod( currwavetime ) );
    }

    if ( isRollover( lastwavetime, currwavetime ) ) {
      setResult( ReadResult::END_OF_DAY );
      prevtime = currwavetime;
      startSaving( );
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
    label = attributes[v8 ? "Label" : "Channel"];
    uom = attributes["UOM"];
    if ( v8 ) {
      v8samplerate = attributes["SampleRate"];
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
    else if ( "FamilyType" == element ) {
      isphilips = ( "Philips" == text );
    }
    else if ( "Size" != element ) {
      filler->metadata( )[element] = text;
    }
  }
  else if ( INNAME == state ) {
    if ( 0 == prevtime ) {
      filler->metadata( )["Patient Name"] = text;
    }
    else {
      std::string pname = filler->metadata( )["Patient Name"];
      if ( text != pname ) {
        setResult( ReadResult::END_OF_PATIENT );
        // we've cut over to a new set of data, so
        // save the data we parse now to a different SignalSet
        startSaving( );
        saved.metadata( )["Patient Name"] = text;
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
    else if ( "VS" == element || "VitalSign" == element ) {
      bool added = false;

      // v8 has a lot of stuff in value that isn't really a value
      // so we need to check that we have a number here
      try {
        std::stof( value );
        std::unique_ptr<SignalData>& sig = filler->addVital( label, &added );

        if ( added ) {
          sig->metad( ).insert( std::make_pair( SignalData::HERTZ, ( isphilips ? 1.0 : 0.5 ) ) );
          sig->metas( ).insert( std::make_pair( SignalData::MSM, MISSING_VALUESTR ) );
          sig->metas( ).insert( std::make_pair( SignalData::TIMEZONE, "UTC" ) );

          if ( !uom.empty( ) ) {
            sig->setUom( uom );
          }
        }
        sig->add( DataRow( datemod( currvstime ), value, "", "", attrs ) );
      }
      catch ( std::invalid_argument ) {
        // don't really care since we're not adding the data to our dataset
        // value = MISSING_VALUESTR;
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
      else if ( shouldExtract( label ) ) {
        std::string wavepoints = text;
        // reverse the oversampling (if any) and set the true Hz
        int thz = 240;
        if ( v8 ) {
          thz = std::stoi( v8samplerate );
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
            sig->metas( ).insert( std::make_pair( SignalData::MSM, MISSING_VALUESTR ) );
            sig->metas( ).insert( std::make_pair( SignalData::TIMEZONE, "UTC" ) );
            // Stp always reads in 1s increments for Philips, 2s for GE monitors
            sig->setValuesPerDataRow( isphilips ? hz : 2 * hz );

            if ( !uom.empty( ) ) {
              sig->setUom( uom );
            }
          }

          sig->metad( )[SignalData::HERTZ] = hz;          
          sig->add( DataRow( datemod( currwavetime ), wavepoints, "", "", attrs ) );          
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
