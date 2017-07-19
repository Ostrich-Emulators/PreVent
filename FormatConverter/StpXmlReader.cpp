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
#include <map>

const std::string StpXmlReader::MISSING_VALUESTR( "-32768" );

StpXmlReader::StpXmlReader( ) {
  LIBXML_TEST_VERSION
}

StpXmlReader::StpXmlReader( const StpXmlReader& orig ) {
}

StpXmlReader::~StpXmlReader( ) {
}

void StpXmlReader::finish( ) {
  if ( NULL == reader ) {
    xmlFreeTextReader( reader );
    reader = NULL;
  }
}

int StpXmlReader::getSize( const std::string& input ) const {
  struct stat info;

  if ( stat( input.c_str( ), &info ) < 0 ) {
    perror( input.c_str( ) );
    return -1;
  }

  return info.st_size;
}

int StpXmlReader::prepare( const std::string& input, ReadInfo& info ) {
  if ( NULL == reader ) {
    reader = xmlNewTextReaderFilename( input.c_str( ) );
  }
  return ( NULL == reader ? -1 : 0 );
}

ReadResult StpXmlReader::readChunk( ReadInfo & info ) {
  ReadResult rslt = ReadResult::NORMAL;
  do {
    rslt = processNode( info );
  }
  while ( rslt == ReadResult::NORMAL );

  return rslt;
}

ReadResult StpXmlReader::processNode( ReadInfo& info ) {
  int ret = next( );
  if ( ret < 0 ) {
    return ReadResult::ERROR;
  }
  else if ( 0 == ret ) {
    return ReadResult::END_OF_FILE;
  }

  int nodetype = xmlTextReaderNodeType( reader );
  int depth = xmlTextReaderDepth( reader );

  std::string element( stringAndFree( xmlTextReaderName( reader ) ) );

  if ( 1 == nodetype ) { // Open element
    if ( 1 == depth ) {
      // only FileInfo, Segment_*, and (sometimes) PatientName elements at level 1
      if ( "FileInfo" == element ) {
        auto m = getHeaders( );

        m.erase( "Size" ); // Size is meaningless for us
        info.metadata( ).insert( m.begin( ), m.end( ) );

        //for ( auto mx : m ) {
        //  std::cout << mx.first << ": " << mx.second << std::endl;
        //}

        next( ); // move past the closing FileInfo tag
      }
      else if ( "PatientName" == element ) {
        // we can safely ignore this (I hope)
      }
      else {
        // Just opened a Segment element: 
        // if we have a PatientName element, then we need to check for rollover
        // if not, we need to check if the VitalSign/Waveform time causes one
        std::string ele = nextelement( );
        if ( "PatientName" == ele ) {
          return handleSegmentPatientName( info );
        }
        else if ( "VitalSigns" == ele ) {
          if ( isRollover( ) ) {
            return ReadResult::END_OF_DAY;
          }
          else {
            handleVitalsSet( info );
          }
        }
        else if ( "Waveforms" == ele ) {
          if ( isRollover( ) ) {
            return ReadResult::END_OF_DAY;
          }
          else {
            handleWaveformSet( info );
          }
        }
        else {
          std::cerr << "unexpected node: " << ele << std::endl;
        }
      }
    }
    else if ( 2 == depth
        && ( "VitalSigns" == element || "Waveforms" == element ) ) {
      // at level 2, it's either VitalSigns or Waveforms elements
      if ( isRollover( ) ) {
        return ReadResult::END_OF_DAY;
      }
      else {
        if ( "VitalSigns" == element ) {
          handleVitalsSet( info );
        }
        else if ( "Waveforms" == element ) {
          handleWaveformSet( info );
        }
        else {
          std::cerr << "unexpected node: " << element << std::endl;
          return ReadResult::ERROR;
        }
      }
    }
  }
  else {
    // std::cout << "element: " << element << std::endl;
  }

  return ReadResult::NORMAL;
}

ReadResult StpXmlReader::handleSegmentPatientName( ReadInfo& info ) {
  std::string patientname = textAndClose( );

  // we have a new patient name, so see if it's the same as our last one
  if ( 0 == prevtime ) {
    // no previous time, so this is our first patient name. save it
    info.metadata( )["Patient Name"] = patientname;
  }
  else if ( 0 == info.metadata( ).count( "Patient Name" ) ||
      ( 1 == info.metadata( ).count( "Patient Name" )
      && info.metadata( )["Patient Name"] != patientname ) ) {
    // didn't have a patient name, but now we do, or we had one and
    // it's different from this new one
    // in either case, rollover
    return ReadResult::END_OF_PATIENT;
  }

  std::cout << "patient name is " << patientname << std::endl;
  return ReadResult::NORMAL;
}

std::string StpXmlReader::textAndClose( ) {
  std::string ret = text( );
  int nodetype = xmlTextReaderNodeType( reader );
  const int MYDEPTH = xmlTextReaderDepth( reader );
  do {
    nodetype = next( );
    // ignore everything until the next start element
  }
  while ( xmlTextReaderDepth( reader ) > MYDEPTH
      && xmlReaderTypes::XML_READER_TYPE_END_ELEMENT != nodetype );

  return ret;
}

bool StpXmlReader::isRollover( ) {
  // check the time against our previous time
  std::map<std::string, std::string> map = getAttrs( );
  currtime = std::stol( map["Time"] );

  if ( 0 == prevtime ) {
    firsttime = currtime;
  }
  else {
    const int cdoy = gmtime( &currtime )->tm_yday;
    const int pdoy = gmtime( &prevtime )->tm_yday;
    if ( cdoy != pdoy ) {
      return true;
    }
  }

  return false;
}

void StpXmlReader::handleWaveformSet( ReadInfo& info ) {
  next( );
  std::string element = stringAndFree( xmlTextReaderName( reader ) );
  while ( "WaveformData" == element ) {
    std::map<std::string, std::string> map;
    std::string vals = textAndAttrsToClose( map );

    bool first;
    std::unique_ptr<SignalData>& sig = info.addWave( map["Channel"], &first );
    if ( first ) {
      sig->metas( ).insert( std::make_pair( SignalData::MSM, MISSING_VALUESTR ) );
      sig->metas( ).insert( std::make_pair( SignalData::TIMEZONE, "UTC" ) );

      if ( 0 != map.count( "UOM" ) ) {
        sig->setUom( map["UOM"] );
      }
    }

    // FIXME: reverse the oversampling (if any) and set the true Hz
    sig->metad( ).insert( std::make_pair( SignalData::HERTZ, 1 / 240 ) );

    sig->add( DataRow( currtime, vals ) );

    next( );
    element = stringAndFree( xmlTextReaderName( reader ) );
  }
}

DataRow StpXmlReader::handleOneVs( std::string& param, std::string& uom ) {
  std::string val = MISSING_VALUESTR;
  std::string hi = MISSING_VALUESTR;
  std::string lo = MISSING_VALUESTR;

  const int MINDEPTH = xmlTextReaderDepth( reader );
  next( ); // opening <Par>, <Value>, etc.
  while ( MINDEPTH < xmlTextReaderDepth( reader ) ) { // consume all the nodes until we get to the closing </VS>
    std::string element = stringAndFree( xmlTextReaderName( reader ) );

    if ( "Value" == element ) {
      std::map<std::string, std::string> attrs;
      val = textAndAttrsToClose( attrs );
      uom = attrs["UOM"];
    }
    else {
      std::string value = textAndClose( );
      if ( "Par" == element ) {
        param = value;
      }
      else if ( "AlarmLimitHigh" == element ) {
        hi = value;
      }
      else if ( "AlarmLimitLow" == element ) {
        lo = value;
      }
    }

    next( ); // could be </VS> or the next <Par>
  }

  return DataRow( currtime, val, hi, lo );
}

int StpXmlReader::next( ) {
  int ret = xmlTextReaderRead( reader );
  if ( ret != 1 ) {
    if( ret < 0 ){
      std::cerr << "error reading stream!" << std::endl;
    }
    return ret; // FIXME: this is confusing for the caller
  }

  int type = xmlTextReaderNodeType( reader );

  if ( xmlReaderTypes::XML_READER_TYPE_SIGNIFICANT_WHITESPACE == type ) {
    //std::cout << "skipping whitespace" << std::endl;
    return next( );
  }

  int depth = xmlTextReaderDepth( reader );
  //std::cout << depth << ":";
  for ( int i = 0; i < depth; i++ ) {
    //std::cout << "  ";
  }

  std::map<int, std::string> typelkp;
  typelkp[xmlReaderTypes::XML_READER_TYPE_ELEMENT] = "open";
  typelkp[xmlReaderTypes::XML_READER_TYPE_END_ELEMENT] = "close";
  typelkp[xmlReaderTypes::XML_READER_TYPE_SIGNIFICANT_WHITESPACE] = "sig white";
  typelkp[xmlReaderTypes::XML_READER_TYPE_TEXT] = "text";
  typelkp[xmlReaderTypes::XML_READER_TYPE_ATTRIBUTE] = "attr";
  //typelkp[xmlReaderTypes]="";

  //  std::cout << stringAndFree( xmlTextReaderName( reader ) )
  //      << ": ->" << stringAndFree( xmlTextReaderValue( reader ) ) << "<-"
  //      << "(" << ( 0 == typelkp.count( type )
  //      ? "type " + std::to_string( type )
  //      : typelkp[type] )
  //      << ")" << std::endl;
  return type;
}

std::string StpXmlReader::textAndAttrsToClose( std::map<std::string, std::string>& attrs ) {
  if ( xmlReaderTypes::XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType( reader ) ) {
    attrs = getAttrs( );
  }
  else {
    std::cerr << "cannot read attributes from non-opening element" << std::endl;
  }

  return textAndClose( );
}

void StpXmlReader::handleVitalsSet( ReadInfo & info ) {
  next( );
  std::string element = stringAndFree( xmlTextReaderName( reader ) );
  while ( "VS" == element ) {
    std::string param;
    std::string uom;
    DataRow row = handleOneVs( param, uom );

    bool first;
    std::unique_ptr<SignalData>& sig = info.addVital( param, &first );
    if ( first ) {
      sig->metad( ).insert( std::make_pair( SignalData::HERTZ, 0.5 ) );
      sig->metas( ).insert( std::make_pair( SignalData::MSM, MISSING_VALUESTR ) );
      sig->metas( ).insert( std::make_pair( SignalData::TIMEZONE, "UTC" ) );

      if ( !uom.empty( ) ) {
        sig->setUom( uom );
      }
    }
    sig->add( row );

    next( );
    element = stringAndFree( xmlTextReaderName( reader ) );
  }
}

std::string StpXmlReader::trim( std::string & totrim ) const {
  // ltrim
  totrim.erase( totrim.begin( ), std::find_if( totrim.begin( ), totrim.end( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );

  // rtrim
  totrim.erase( std::find_if( totrim.rbegin( ), totrim.rend( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), totrim.end( ) );

  return totrim;
}

std::map<std::string, std::string> StpXmlReader::getHeaders( ) {
  std::map<std::string, std::string> map;

  const int MINDEPTH = xmlTextReaderDepth( reader );

  next( ); // opening <Filename>, <Unit>, etc.
  while ( MINDEPTH < xmlTextReaderDepth( reader ) ) {
    // consume all the nodes until we get to the closing </FileInfo>... (our MINDEPTH)
    std::string element = stringAndFree( xmlTextReaderName( reader ) );
    std::string value = textAndClose( );
    map.insert( std::make_pair( element, value ) );

    next( ); // could be </FileInfo> or the next opening <Unit>
  }

  return map;
}

std::string StpXmlReader::nextelement( ) {
  // ignore everything until the next start element
  while ( xmlReaderTypes::XML_READER_TYPE_ELEMENT != next( ) ) {
    // nothing to do in the loop...the next() does it all
  }

  return stringAndFree( xmlTextReaderName( reader ) );
}

std::string StpXmlReader::text( ) {
  next( );
  std::string value = stringAndFree( xmlTextReaderValue( reader ) );

  return trim( value );
}

std::string StpXmlReader::stringAndFree( xmlChar * chars ) const {
  std::string ret;
  if ( NULL != chars ) {
    ret = std::string( (char *) chars );
    xmlFree( chars );
  }
  return ret;
}

std::map<std::string, std::string> StpXmlReader::getAttrs( ) {
  std::map<std::string, std::string> map;
  if ( xmlTextReaderHasAttributes( reader ) ) {
    while ( xmlTextReaderMoveToNextAttribute( reader ) ) {
      std::string key = stringAndFree( xmlTextReaderName( reader ) );
      std::string val = stringAndFree( xmlTextReaderValue( reader ) );
      map[key] = val;
    }
  }
  return map;
}
