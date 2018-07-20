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

#include "CpcXmlReader.h"
#include "SignalData.h"
#include "Base64.h"

#include <fstream>
#include <sys/stat.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <map>

const std::set<std::string> CpcXmlReader::ignorables
{
  "formatID", "sessionID", "blockSQN", "blockLength",
  "startDateTime", "StartTime", "SessStatus", "MODE",
  "mean"
};

CpcXmlReader::CpcXmlReader( ) : XmlReaderBase( "CPC XML" ), currtime( 0 ), lasttime( 0 ) {
}

CpcXmlReader::CpcXmlReader( const CpcXmlReader& orig ) : XmlReaderBase( orig ),
currtime( 0 ), lasttime( 0 ) {
}

CpcXmlReader::~CpcXmlReader( ) {
}

void CpcXmlReader::comment( const std::string& text ) {
  // nothing to do  
}

void CpcXmlReader::start( const std::string& element,
        std::map<std::string, std::string>& attrs ) {
  if ( "cpc" == element ) {
    lasttime = currtime;

    size_t decimalplace = attrs["datetime"].find( "." );
    //output( ) << "XXX " << attrs["datetime"] << std::endl;
    currtime = time( attrs["datetime"].substr( 0, decimalplace ) );
    //output( ) << currtime << std::endl;

    if ( isRollover( lasttime, currtime ) ) {
      setResult( ReadResult::END_OF_DAY );
      startSaving( currtime );
    }
    inmg = false;
  }
  else if ( "device" == element ) {
    for ( auto& x : attrs ) {
      filler->setMeta( x.first, x.second );
    }
  }
  else if ( "m" == element ) {
    if ( inmg ) {
      inwave = ( "Wave" == attrs["name"] );
      inhz = ( "Hz" == attrs["name"] );
      inpoints = ( "Points" == attrs["name"] );
      ingain = ( "Gain" == attrs["name"] );
    }
    else {
      if ( 0 == ignorables.count( attrs["name"] ) ) {
        label = attrs["name"];
      }
      // else ignored
    }
  }
  else if ( "mg" == element ) {
    inmg = true;
    label = attrs["name"];
  }
}

void CpcXmlReader::end( const std::string& element, const std::string& text ) {
  if ( "mg" == element ) {
    bool added = false;

    std::vector<BYTE> data = base64_decode( value );
    std::string vals;
    for ( int i = 0; i < data.size( ); i += 2 ) {
      BYTE one = data[i];
      BYTE two = data[i + 1];
      short val = ( ( two << 8 ) | one ); // litle-endian (from trial-and-error)

      if ( !vals.empty( ) ) {
        vals.append( "," );
      }
      vals.append( std::to_string( val ) );
    }

    std::unique_ptr<SignalData>& signal = filler->addWave( label, &added );
    signal->add( DataRow( currtime, vals ) );
    if ( added ) {
      signal->setChunkIntervalAndSampleRate( 2000, valsperdr );
      signal->setMeta( "Gain", gain );
    }

    inmg = inhz = inpoints = false;
    label.clear( );
  }
  else if ( "m" == element ) {
    if ( label.empty( ) || text.empty( ) ) {
      return;
    }

    if ( inmg ) {
      if ( inwave ) {
        // we have wave data to decode
        value = text;
      }
      else if ( inhz ) {
        wavehz = std::stod( text );
      }
      else if ( inpoints ) {
        valsperdr = std::stoi( text );
      }
      else if ( ingain ) {
        gain = std::stod( text );
      }
    }
    else {
      bool added = false;
      std::unique_ptr<SignalData>& signal = filler->addVital( label, &added );
      signal->add( DataRow( currtime, text ) );

      if ( added ) {
        signal->setChunkIntervalAndSampleRate( 2000, 1 );
      }
      label.clear( );
    }
  }
}
