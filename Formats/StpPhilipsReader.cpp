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

#include "StpPhilipsReader.h"
#include "SignalData.h"
#include "DataRow.h"
#include "BasicSignalSet.h"
#include "SignalUtils.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <filesystem>
#include "config.h"
#include "CircularBuffer.h"
#include <expat.h>

namespace FormatConverter{

  StpPhilipsReader::StpPhilipsReader( const std::string& name ) : StpReaderBase( name ) { }

  StpPhilipsReader::StpPhilipsReader( const StpPhilipsReader& orig ) : StpReaderBase( orig ) { }

  StpPhilipsReader::~StpPhilipsReader( ) { }

  int StpPhilipsReader::prepare( const std::string& filename, std::unique_ptr<SignalSet>& data ) {
    int rslt = StpReaderBase::prepare( filename, data );
    return rslt;
  }

  ReadResult StpPhilipsReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {
    output( ) << "initial reading from input stream (popped:" << work.popped( ) << ")" << std::endl;

    if ( ReadResult::END_OF_PATIENT == lastrr ) {
      info->setMeta( "Patient Name", "" ); // reset the patient name
    }

    int cnt = StpReaderBase::readMore( );
    if ( cnt < 0 ) {
      return ReadResult::ERROR;
    }

    std::string patientId;
    while ( cnt > 0 ) {
      std::cout << "popped: " << work.popped( ) << "; work buffer size: " << work.size( ) << "; available:" << work.available( ) << std::endl;
      if ( work.available( ) < 1024 * 768 ) {
        // we should never come close to filling up our work buffer
        // so if we have, make sure the user knows
        std::cerr << "work buffer is too full...something is going wrong" << std::endl;
        return ReadResult::ERROR;
      }

      // read as many segments as we can before reading more data
      ChunkReadResult rslt = ChunkReadResult::OK;
      std::string xmldoc;
      std::string rootelement;

      while ( hasCompleteXmlDoc( xmldoc, rootelement ) && ChunkReadResult::OK == rslt ) {
        //output( ) << "next segment is " << std::dec << segsize << " bytes big" << std::endl;
        XML_Parser parser = XML_ParserCreate( NULL );
        xmlpassthru passthru = { info, ParseState::UNINTERESTING, .outer = *this };
        XML_SetUserData( parser, &passthru );
        //        output( ) << "root element is: " << rootelement << std::endl;
        //        output( ) << xmldoc << std::endl << std::endl;
        //        output( ) << "doc is " << xmldoc.length( ) << " big" << std::endl;

        if ( "PatientUpdateResponse" == rootelement ) {
          XML_SetElementHandler( parser, StpPhilipsReader::PatientParser::start, StpPhilipsReader::PatientParser::end );
          XML_SetCharacterDataHandler( parser, StpPhilipsReader::PatientParser::chars );
        }
        else {
          XML_SetElementHandler( parser, StpPhilipsReader::DataParser::start, StpPhilipsReader::DataParser::end );
          XML_SetCharacterDataHandler( parser, StpPhilipsReader::DataParser::chars );
        }

        XML_Status status = XML_Parse( parser, xmldoc.c_str( ), xmldoc.length( ), true );
        if ( status != XML_STATUS_OK ) {
          XML_Error err = XML_GetErrorCode( parser );
          std::cerr << XML_ErrorString( err )
              << " line: " << XML_GetCurrentLineNumber( parser )
              << " column: " << XML_GetCurrentColumnNumber( parser )
              << std::endl;
          return ReadResult::ERROR;
        }

        XML_ParserFree( parser );
        xmldoc.clear( );
        rootelement.clear( );

        // FIXME: this puts the first data point of the new patient with the
        // old patient's data! Check PatientId earlier, and rewind our buffer
        // if we need to use END_OF_PATIENT
        if ( patientId.empty( ) ) {
          patientId = passthru.currentPatientId;
        }
        else if ( patientId != passthru.currentPatientId ) {
          return ReadResult::END_OF_PATIENT;
        }
      }

      xmldoc.clear( );
      rootelement.clear( );
      cnt = readMore( );
      if ( cnt < 0 ) {
        return ReadResult::ERROR;
      }
    }

    output( ) << "file is exhausted" << std::endl;

    // copy any data we have left in our filler set to the real set

    // if we still have stuff in our work buffer, process it
    if ( !work.empty( ) ) {
      //output( ) << "still have stuff in our work buffer!" << std::endl;
      processOneChunk( info, work.size( ) );
    }

    return ReadResult::END_OF_FILE;
  }

  void StpPhilipsReader::skipUntil( const std::string& needle ) {
    while ( !work.empty( ) ) {
      while ( work.pop( ) != needle[0] ) {
        // nothing to do here, just wanted to pop
      }

      // found the first character, so see if we found the whole string
      std::string str = readString( needle.size( ) - 1 );
      if ( needle.substr( 1 ) == str ) {
        work.rewind( ); // rewind to the first character
        break;
      }
    }
  }

  StpPhilipsReader::ChunkReadResult StpPhilipsReader::processOneChunk( std::unique_ptr<SignalSet>& info,
      const size_t& maxread ) {
    return ChunkReadResult::OK;
  }

  dr_time StpPhilipsReader::popTime( ) {
    // time is in little-endian format
    auto shorts = work.popvec( 4 );
    time_t time = ( ( shorts[1] << 24 ) | ( shorts[0] << 16 ) | ( shorts[3] << 8 ) | shorts[2] );
    return time * 1000;
  }

  bool StpPhilipsReader::hasCompleteXmlDoc( std::string& found, std::string& rootelement ) {
    // ensure that our data has at least one xml header ("<?xml->..>")
    // and closes the root element (there is junk afterwards)
    if ( work.empty( ) ) {
      found = false;
      return "";
    }

    bool ok = false;
    work.mark( );
    size_t markpop = work.popped( );
    try {
      skipUntil( "<?xml" );
      size_t xmlstart = work.popped( ) - markpop;

      work.skip( ); // skip the '<' of <?xml
      skipUntil( "<" ); // find the root element
      size_t rootElementNameStart = work.popped( ) + 1;
      skipUntil( ">" );
      size_t rootElementNameStop = work.popped( );

      size_t rootElementNameLength = ( rootElementNameStop - rootElementNameStart );
      work.rewind( rootElementNameLength );

      auto rootpieces = SignalUtils::splitcsv( popString( rootElementNameLength ), ' ' );
      rootelement.append( rootpieces[0] );

      std::string endneedle( "</" + rootelement + ">" );
      skipUntil( endneedle );
      skipUntil( ">" );
      work.skip( );
      size_t xmlend = work.popped( );

      work.rewindToMark( );
      work.skip( xmlstart );

      size_t doclength = ( ( xmlend - markpop ) - xmlstart );
      std::string temp( popString( doclength ) );

      if ( temp[temp.size( ) - 1] != '>' ) {
        // FIXME: why do we end up with one extra character on every
        // read but the first?!
        work.rewind( );
        temp.erase( temp.size( ) - 1 );
      }

      //std::cout << "\tmark at: " << markpop << "; " << xmlstart << " bytes of junk first; xml doc: "
      //    << ( markpop + xmlstart ) << "-" << xmlend << " (popped: " << work.popped( ) << ")" << std::endl;
      found.append( temp );
      ok = true;
    }
    catch ( const std::runtime_error& x ) {
      // most likely, we've tried to read past the end of our buffer
      // don't really care, but don't throw anything, and rewind so we can
      // fill in more data later without losing what we have in the chute
      work.rewindToMark( );
      //std::cout << "\trewound to " << work.popped( ) << std::endl;
    }

    return ok;
  }

  std::vector<StpReaderBase::StpMetadata> StpPhilipsReader::parseMetadata( const std::string& input ) {
    std::vector<StpReaderBase::StpMetadata> metas;
    std::unique_ptr<SignalSet> info( new BasicSignalSet( ) );
    StpPhilipsReader reader;
    reader.setNonbreaking( true );
    int failed = reader.prepare( input, info );
    if ( failed ) {
      std::cerr << "error while opening input file. error code: " << failed << std::endl;
      return metas;
    }
    reader.setMetadataOnly( );

    ReadResult last = ReadResult::FIRST_READ;

    bool okToContinue = true;
    while ( okToContinue ) {
      last = reader.fill( info, last );
      switch ( last ) {
        case ReadResult::FIRST_READ:
          // NOTE: no break here
        case ReadResult::NORMAL:
          break;
        case ReadResult::END_OF_DAY:
          metas.push_back( metaFromSignalSet( info ) );
          info->reset( false );
          break;
        case ReadResult::END_OF_PATIENT:
          metas.push_back( metaFromSignalSet( info ) );
          info->reset( false );
          break;
        case ReadResult::END_OF_FILE:
          metas.push_back( metaFromSignalSet( info ) );
          info->reset( false );
          okToContinue = false;
          break;
        case ReadResult::ERROR:
          std::cerr << "error while reading input file" << std::endl;
          okToContinue = false;
          break;
      }
    }

    reader.finish( );

    return metas;
  }

  StpReaderBase::StpMetadata StpPhilipsReader::metaFromSignalSet( const std::unique_ptr<SignalSet>& info ) {
    StpReaderBase::StpMetadata meta;
    if ( info->metadata( ).count( "Patient Name" ) > 0 ) {
      meta.name = info->metadata( ).at( "Patient Name" );
    }
    if ( info->metadata( ).count( "MRN" ) > 0 ) {
      meta.mrn = info->metadata( ).at( "MRN" );
    }

    if ( info->offsets( ).size( ) > 0 ) {
      meta.segment_count = (int) ( info->offsets( ).size( ) );
      std::vector<dr_time> times;
      for ( auto en : info->offsets( ) ) {
        times.push_back( en.second );
      }
      std::sort( times.begin( ), times.end( ) );
      meta.start_utc = times[0];
      meta.stop_utc = times[info->offsets( ).size( ) - 1];
    }

    return meta;
  }

  dr_time StpPhilipsReader::parseTime( const std::string& datetime ) {
    int y, M, d, h, m, s, ms;
    int tzh = 0;
    int tzm = 0;

    int parsed = std::sscanf( datetime.c_str( ), "%d-%d-%dT%d:%d:%d.%dZ", &y, &M, &d, &h, &m, &s, &ms );
    if ( parsed > 6 ) {
      // got hours-based timezone, no 'Z'
      if ( tzh < 0 ) {
        tzm = -tzm;
      }
    }

    tm time;
    time.tm_year = y - 1900;
    time.tm_mon = M - 1;
    time.tm_mday = d;
    time.tm_hour = h;
    time.tm_min = m;
    time.tm_sec = s;
    time.tm_gmtoff = tzh;

    time_t tt = mktime( &time );
    dr_time newtime = tt * 1000 + ms;
    return Reader::modtime( newtime );
  }

  void StpPhilipsReader::PatientParser::start( void * data, const char * el, const char ** attr ) {
    if ( 0 == strcmp( "PatientInfo", el ) ) {
      ( (xmlpassthru*) data )->state = ParseState::PATIENTINFO;
    }
  }

  void StpPhilipsReader::PatientParser::end( void * data, const char * el ) {
    xmlpassthru * xml = (xmlpassthru*) data;
    if ( ParseState::PATIENTINFO == xml->state ) {
      if ( 0 == strcmp( "Id", el ) ) {
        xml->currentPatientId = xml->currentText;
      }
      else if ( 0 == strcmp( "DisplayName", el ) ) {
        xml->signals->setMeta( "Patient Name", xml->currentText );
      }
      else if ( 0 == strcmp( "DateOfBirth", el ) ) {
        xml->signals->setMeta( "DOB", std::to_string( xml->outer.parseTime( xml->currentText ) ) );
        xml->signals->setMeta( el, xml->currentText );
      }
      else if ( 0 == strcmp( "PrimaryId", el ) || 0 == strcmp( "LifetimeId", el ) ) {
        xml->signals->setMeta( el, xml->currentText );
      }
    }
    else {
      xml->state = ParseState::UNINTERESTING;
    }
  }

  void StpPhilipsReader::PatientParser::chars( void * data, const char * text, int len ) {
    std::string chardata( text, len );
    SignalUtils::trim( chardata );
    if ( !chardata.empty( ) ) {
      ( *(xmlpassthru*) data ).currentText = chardata;
    }
  }

  void StpPhilipsReader::DataParser::start( void * data, const char * el, const char ** attr ) {
    xmlpassthru * xml = (xmlpassthru*) data;
    if ( 0 == strcmp( "NumericCompound", el ) ) {
      xml->state = ParseState::NUMERICCOMPOUND;
    }
    else if ( 0 == strcmp( "NumericValue", el ) ) {
      xml->state = ParseState::NUMERICVALUE;
    }
    else if ( 0 == strcmp( "NumericAttribute", el ) ) {
      xml->state = ParseState::NUMERICATTR;
    }
    else if ( 0 == strcmp( "Wave", el ) ) {
      xml->state = ParseState::WAVE;
    }
  }

  void StpPhilipsReader::DataParser::end( void * data, const char * el ) {
    //    std::cout << "data end: " << el << std::endl;
    xmlpassthru * xml = (xmlpassthru*) data;
    if ( 0 == strcmp( "NumericCompound", el ) ) {
      bool added = false;
      auto& vital = xml->signals->addVital( xml->label, &added );
      if ( added ) {
        vital->setUom( xml->uom );
        vital->setChunkIntervalAndSampleRate( 1024, 1 );
      }
      vital->add( DataRow( xml->currentTime, xml->value ) );
      std::cout << "save vital data: " << xml->currentTime << " " << xml->label << " (" << xml->uom << ") " << xml->value << std::endl;
    }
    else if ( 0 == strcmp( "WaveSegment", el ) ) {
      bool added = false;
      auto& wave = xml->signals->addWave( xml->label, &added );
      if ( added ) {
        wave->setChunkIntervalAndSampleRate( 1024, 1024 / std::stoi( xml->sampleperiod ) );
      }
      // wave->add( DataRow( xml->currentTime, xml->value ) );
      std::cout << "save wave data: " << xml->currentTime << " " << xml->label << " (" << xml->sampleperiod << ") " << xml->value << std::endl;
    }
    else if ( ParseState::WAVE == xml->state ) {
      if ( 0 == strcmp( "Label", el ) ) {
        xml->label = xml->currentText;
      }
      else if ( 0 == strcmp( "SamplePeriod", el ) ) {
        xml->sampleperiod = xml->currentText;
      }
      else if ( 0 == strcmp( "WaveSamples", el ) ) {
        xml->value = xml->currentText;
      }
    }
    else if ( ParseState::NUMERICCOMPOUND == xml->state ) {
      if ( 0 == strcmp( "Time", el ) ) {
        xml->currentTime = xml->outer.parseTime( xml->currentText );
      }
    }
    else if ( ParseState::NUMERICVALUE == xml->state ) {
      if ( 0 == strcmp( "Label", el ) ) {
        xml->label = xml->currentText;
      }
      else if ( 0 == strcmp( "Value", el ) ) {
        xml->value = xml->currentText;
      }
    }
    else if ( ParseState::NUMERICATTR == xml->state ) {
      if ( 0 == strcmp( "UnitLabel", el ) ) {
        xml->uom = xml->currentText;
      }
    }
    else {
      xml->state = ParseState::UNINTERESTING;
    }
  }

  void StpPhilipsReader::DataParser::chars( void * data, const char * text, int len ) {
    return PatientParser::chars( data, text, len );
  }
}

