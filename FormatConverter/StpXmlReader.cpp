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

const std::string StpXmlReader::MISSING_VALUESTR( "-32768" );
//ReadResult StpXmlReader::rslt = ReadResult::NORMAL;
//std::string StpXmlReader::workingText;
//std::string StpXmlReader::element;
//std::string StpXmlReader::last;
//std::map<std::string, std::string> StpXmlReader::attrs;
//DataRow StpXmlReader::current;
//StpXmlReaderState StpXmlReader::state = StpXmlReaderState::OTHER;
//time_t StpXmlReader::firsttime = 0;
//time_t StpXmlReader::prevtime = 0;
//std::list<ReadInfo> StpXmlReader::buffered;

StpXmlReader::StpXmlReader( ) {
  LIBXML_TEST_VERSION
}

StpXmlReader::StpXmlReader( const StpXmlReader& orig ) {
}

StpXmlReader::~StpXmlReader( ) {
}

void StpXmlReader::finish( ) {
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
  reader = xmlNewTextReaderFilename( input.c_str( ) );
  return ( NULL == reader ? -1 : 0 );
}

ReadResult StpXmlReader::readChunk( ReadInfo & info ) {
  int ret = xmlTextReaderRead( reader );
  while ( ret == 1 ) {
    processNode( reader, info );
    ret = xmlTextReaderRead( reader );
  }
  xmlFreeTextReader( reader );

  return ReadResult::END_OF_FILE;
}

ReadResult StpXmlReader::processNode( xmlTextReaderPtr reader, ReadInfo& info ) {
  xmlChar * name = xmlTextReaderName( reader );
  int nodetype = xmlTextReaderNodeType( reader );
  int depth = xmlTextReaderDepth( reader );

  std::string element( (char *) name );
  xmlFree( name );

  if ( 1 == nodetype ) { // Open element
    if ( 1 == depth ) {
      // only FileInfo, Segment_*, and (sometimes) PatientName elements at level 1
      if ( "FileInfo" == element ) {

        auto m = getHeaders( xmlTextReaderExpand( reader ) );
        m.erase( "Size" );
        info.metadata( ).insert( m.begin( ), m.end( ) );

        for ( auto mx : m ) {
          std::cout << mx.first << ": " << mx.second << std::endl;
        }

      }
      else if ( "PatientName" == element ) {
        // we can safely ignore this (I hope)
      }
      else {
        // Just opened a Segment element
        for ( auto m : getAttrs( reader ) ) {
          std::cout << m.first << ": " << m.second << std::endl;
        }
      }
    }
    else {
      //  std::cout << "depth:" << xmlTextReaderDepth( reader )
      //      << "| type:" << xmlTextReaderNodeType( reader )
      //      << "| element:" << element
      //      << "| empty:" << xmlTextReaderIsEmptyElement( reader )
      //      << "| val: ->" << value << "<-" << std::endl;

    }
  }


  return ReadResult::NORMAL;
}

std::string StpXmlReader::trim( std::string& totrim ) const {
  // ltrim
  totrim.erase( totrim.begin( ), std::find_if( totrim.begin( ), totrim.end( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );

  // rtrim
  totrim.erase( std::find_if( totrim.rbegin( ), totrim.rend( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), totrim.end( ) );

  return totrim;
}

std::map<std::string, std::string> StpXmlReader::getHeaders( xmlNodePtr node ) const {
  std::map<std::string, std::string> map;
  xmlNodePtr cur = node->children;
  while ( NULL != cur ) {
    std::string key( (char *) cur->name );
    std::string val = stringAndFree( xmlNodeGetContent( cur ) );
    if ( !( "" == trim( key ) || "" == trim( val ) ) ) {
      map[key] = val;
    }
    cur = cur->next;
  }

  return map;
}

std::string StpXmlReader::nextelement( xmlTextReaderPtr reader ) const {
  xmlTextReaderRead( reader ); // next opening element?
  int nodetype = xmlTextReaderNodeType( reader );
  if ( xmlReaderTypes::XML_READER_TYPE_SIGNIFICANT_WHITESPACE == nodetype ) {
    // ignore this junk whitespace
    xmlTextReaderRead( reader ); // real opening element (?)
  }

  xmlChar * name = xmlTextReaderName( reader );
  std::string element( (char *) name );
  xmlFree( name );
  return element;
}

std::string StpXmlReader::text( xmlTextReaderPtr reader ) const {
  xmlTextReaderRead( reader );
  std::string value = stringAndFree( xmlTextReaderValue( reader ) );
  return trim( value );
}

DataRow StpXmlReader::getVital( xmlTextReaderPtr reader ) const {
  return DataRow( );
}

std::string StpXmlReader::stringAndFree( xmlChar * chars ) const {
  std::string ret;
  if ( NULL != chars ) {
    ret.assign( (char *) chars );
    xmlFree( chars );
  }
  return ret;
}

std::map<std::string, std::string> StpXmlReader::getAttrs( xmlTextReaderPtr reader ) const {
  std::map<std::string, std::string> map;
  if ( xmlTextReaderHasAttributes( reader ) ) {
    while ( xmlTextReaderMoveToNextAttribute( reader ) ){
      std::string key = stringAndFree( xmlTextReaderName( reader ) );
      std::string val = stringAndFree( xmlTextReaderValue( reader) );
      map[key] = val;
    }
  }
  return map;
}

//void StpXmlReader::start( void * ) {
//  rslt = ReadResult::NORMAL;
//}
//
//void StpXmlReader::finish( void * ) {
//  std::cout << "into end do2c" << std::endl;
//}
//
//void StpXmlReader::chars( void *, const xmlChar * ch, int len ) {
//  std::string mychars( (char *) ch, len );
//
//  // ltrim
//  mychars.erase( mychars.begin( ), std::find_if( mychars.begin( ), mychars.end( ),
//      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );
//
//  // rtrim
//  mychars.erase( std::find_if( mychars.rbegin( ), mychars.rend( ),
//      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), mychars.end( ) );
//
//  if ( !mychars.empty( ) ) {
//    workingText += mychars;
//  }
//  rslt = ReadResult::NORMAL;
//}
//
//void StpXmlReader::startElement( void * user_data, const xmlChar * name,
//    const xmlChar ** attrarr ) {
//  int idx = 0;
//  attrs.clear( );
//  if ( NULL != attrarr ) {
//    char * key = (char *) attrarr[idx];
//    while ( NULL != key ) {
//      std::string val( (char *) attrarr[++idx] );
//      attrs[std::string( key )] = val;
//      key = (char *) attrarr[++idx];
//    }
//  }
//
//
//  element.assign( (char *) name );
//  if ( "FileInfo" == element || "PatientName" == element ) {
//    state = StpXmlReaderState::HEADER;
//  }
//  else if ( "VitalSigns" == element ) {
//    state = StpXmlReaderState::VITAL;
//    time_t vtime = std::stol( attrs["Time"] );
//    current.time = vtime;
//
//    if ( 0 != prevtime ) {
//      // check if time is a rollover event!
//      tm * prev = gmtime( &prevtime );
//      tm * now = gmtime( &vtime );
//
//      if ( prev->tm_yday != now->tm_yday ) {
//        // ROLLOVER!
//        rslt = ReadResult::END_OF_DAY;
//        addBufferedData( convertUserDataToReadInfo( user_data ) );
//      }
//    }
//  }
//  else if ( "VS" == element ) {
//    current.data.clear( );
//    current.high = MISSING_VALUESTR;
//    current.low = MISSING_VALUESTR;
//  }
//  else if ( "Waveforms" == element ) {
//    state = StpXmlReaderState::WAVE;
//  }
//
//  rslt = ReadResult::NORMAL;
//}
//
//ReadInfo& StpXmlReader::addBufferedData( ReadInfo& old ) {
//  prevtime = 0;
//  ReadInfo newdata;
//  newdata.metadata( ).insert( old.metadata( ).begin( ), old.metadata( ).end( ) );
//  buffered.push_back( newdata );
//  return buffered.back( );
//}
//
//void StpXmlReader::endElement( void * user_data, const xmlChar * name ) {
//  // check for the header metadata of interest
//  ReadInfo& info = convertUserDataToReadInfo( user_data );
//
//  std::string element( (char *) name );
//  bool cleartext = false;
//
//  if ( StpXmlReaderState::HEADER == state ) {
//    if ( "Filename" == element || "Unit" == element || "Bed" == element ) {
//      info.addMeta( element, workingText );
//    }
//    else if ( "PatientName" == element ) {
//      // check if patient name changed
//      if ( !( 0 == prevtime || 0 == info.metadata( ).count( "PatientName" ) ) ) {
//        std::string oldname = info.metadata( )["PatientName"];
//        if ( oldname != workingText ) {
//          // new patient
//          rslt = ReadResult::END_OF_PATIENT;
//          addBufferedData( info );
//        }
//      }
//    }
//    else if ( "FileInfo" == element ) {
//      state = StpXmlReaderState::OTHER;
//    }
//    cleartext = true;
//  }
//  else if ( StpXmlReaderState::VITAL == state ) {
//    rslt = handleVital( element, info );
//    cleartext = true;
//  }
//  else if ( StpXmlReaderState::WAVE == state ) {
//    rslt = handleWave( element, info );
//    cleartext = true;
//  }
//
//
//  if ( cleartext ) {
//    workingText.clear( );
//  }
//  rslt = ReadResult::NORMAL;
//}
//
//ReadResult StpXmlReader::handleWave( const std::string& element, ReadInfo& info ) {
//  return ReadResult::NORMAL;
//}
//
//ReadResult StpXmlReader::handleVital( const std::string& element, ReadInfo& info ) {
//  if ( "Par" == element ) {
//    std::unique_ptr<SignalData>& sigdata = info.addVital( workingText );
//    sigdata->setUom( "Uncalib" );
//    last = workingText;
//  }
//  else if ( "Value" == element ) {
//    current.data = workingText;
//
//    if ( 0 != attrs.count( "UOM" ) ) {
//      std::unique_ptr<SignalData>& sigdata = info.addVital( last );
//      sigdata->setUom( attrs["UOM"] );
//    }
//  }
//  else if ( "AlarmLimitLow" == element ) {
//    current.low = workingText;
//  }
//  else if ( "AlarmLimitHigh" == element ) {
//    current.high = workingText;
//  }
//  else if ( "VS" == element ) {
//    std::unique_ptr<SignalData>& sigdata = info.addVital( last );
//    sigdata->add( current );
//    last.clear( );
//    prevtime = current.time;
//  }
//}
//
//ReadInfo& StpXmlReader::convertUserDataToReadInfo( void * data ) {
//  ReadInfo * dd = static_cast<ReadInfo *> ( data );
//  return *dd;
//}
//
//void StpXmlReader::error( void *user_data, const char *msg, ... ) {
//  std::cerr << "error: " << msg << std::endl;
//  rslt = ReadResult::ERROR;
//}
