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
#include "config.h"
#include "CircularBuffer.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <filesystem>
#include <expat.h>

namespace FormatConverter{
  const std::string StpPhilipsReader::DataParser::NUMERIC_ATTR = "NumericAttribute";
  const std::string StpPhilipsReader::DataParser::NUMERIC_CMPD = "NumericCompound";
  const std::string StpPhilipsReader::DataParser::NUMERIC_VAL = "NumericValue";
  const std::string StpPhilipsReader::DataParser::WAVE = "Wave";
  const std::string StpPhilipsReader::DataParser::WAVE_SEG = "WaveSegment";

  StpPhilipsReader::StpPhilipsReader( const std::string& name ) : StpReaderBase( name ) { }

  StpPhilipsReader::StpPhilipsReader( const StpPhilipsReader& orig ) : StpReaderBase( orig ) { }

  StpPhilipsReader::~StpPhilipsReader( ) { }

  std::string StpPhilipsReader::peekPatientId( const std::string& xmldoc, bool ispatientdoc ) {
    size_t start;
    size_t end;
    if ( ispatientdoc ) {
      size_t pstarter = xmldoc.find( "<PatientInfo>" );
      start = xmldoc.find( "<Id>", pstarter ) + 4;
      end = xmldoc.find( "</Id>", start );
    }
    else {
      start = xmldoc.find( "<PatientId>" ) + 11;
      end = xmldoc.find( "</PatientId>", start );
    }

    return xmldoc.substr( start, ( end - start ) );
  }

  dr_time StpPhilipsReader::peekTime( const std::string& xmldoc ) {
    size_t start = xmldoc.find( "<Time>" ) + 6;
    size_t end = xmldoc.find( "</Time>", start );
    return ( ( !( std::string::npos == start || std::string::npos == end ) )
        ? parseTime( xmldoc.substr( start, ( end - start ) ) )
        : 0 );
  }

  int StpPhilipsReader::prepare( const std::string& filename, std::unique_ptr<SignalSet>& data ) {
    int rslt = StpReaderBase::prepare( filename, data );
    wavewarning = false;
    return rslt;
  }

  ReadResult StpPhilipsReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {
    currentTime = 0;
    output( ) << "initial reading from input stream (popped:" << work.popped( ) << ")" << std::endl;

    if ( ReadResult::END_OF_PATIENT == lastrr ) {
      info->setMeta( "Patient Name", "" ); // reset the patient name
      info->setMeta( "PrimaryId", "" );
      info->setMeta( "LifetimeId", "" );
      info->setMeta( "DOB", "" );
      info->setMeta( "Date of Birth", "" );
    }

    int cnt = StpReaderBase::readMore( );
    if ( cnt < 0 ) {
      return ReadResult::ERROR;
    }

    std::string patientId;
    int loopcounter = 0;
    while ( cnt > 0 ) {
      if ( 0 == loopcounter % 3000 ) {
        std::cout << "(loop " << loopcounter << ") popped: " << work.popped( ) << "; work buffer size: " << work.size( ) << "; available:" << work.available( ) << std::endl;
      }
      loopcounter++;

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

      size_t beforecheck = work.popped( );
      while ( hasCompleteXmlDoc( xmldoc, rootelement ) && ChunkReadResult::OK == rslt ) {
        //output( ) << "next segment is " << std::dec << segsize << " bytes big" << std::endl;
        //        if ( 2213001668L <= work.popped( ) ) {
        //          std::cout << "get ready: " << work.popped( ) << " " << work.size( ) << std::endl;
        //        }

        bool isPatientDoc = ( "PatientUpdateResponse" == rootelement );


        // if the patient has changed, we need to end this read before processing
        // the XML fully. Same goes with the rollover check
        std::string newpatientid = peekPatientId( xmldoc, isPatientDoc );
        if ( patientId.empty( ) ) {
          patientId = newpatientid;
        }
        else {
          if ( patientId != newpatientid ) {
            work.rewind( work.popped( ) - beforecheck );
            return ReadResult::END_OF_PATIENT;
          }
        }

        dr_time newtime = peekTime( xmldoc );
        if ( 0 != newtime && isRollover( currentTime, newtime ) ) {
          work.rewind( work.popped( ) - beforecheck );
          return ReadResult::END_OF_DAY;
        }
        currentTime = newtime;

        XML_Parser parser = XML_ParserCreate( NULL );
        xmlpassthru passthru = { info, ParseState::UNINTERESTING, .outer = *this };
        XML_SetUserData( parser, &passthru );
        //        output( ) << "root element is: " << rootelement << std::endl;
        //        output( ) << xmldoc << std::endl << std::endl;
        //        output( ) << "doc is " << xmldoc.length( ) << " big" << std::endl;


        if ( isPatientDoc ) {
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

        beforecheck = work.popped( );
        //        if ( 2213001668L <= work.popped( ) ) {
        //          std::cout << "get ready: " << work.popped( ) << " " << work.size( ) << std::endl;
        //        }
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
      return false;
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
        xml->signals->setMeta( "Date of Birth", xml->currentText );
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
    if ( NUMERIC_CMPD == el ) {
      xml->state = ParseState::NUMERICCOMPOUND;
    }
    else if ( NUMERIC_VAL == el ) {
      xml->state = ParseState::NUMERICVALUE;
    }
    else if ( NUMERIC_ATTR == el ) {
      xml->state = ParseState::NUMERICATTR;
    }
    else if ( WAVE == el ) {
      xml->state = ParseState::WAVE;
    }
  }

  void StpPhilipsReader::DataParser::end( void * data, const char * el ) {
    //    std::cout << "data end: " << el << std::endl;
    xmlpassthru * xml = (xmlpassthru*) data;
    if ( NUMERIC_CMPD == el ) {
      bool added = false;
      auto& vital = xml->signals->addVital( xml->label, &added );
      if ( added ) {
        vital->setUom( xml->uom );
        vital->setChunkIntervalAndSampleRate( 1024, 1 );
      }

      // we call stoi to ensure we have an actual value
      // ...if an exception is thrown, just move on
      try {
        std::stoi( xml->value );
        vital->add( DataRow( xml->currentTime, xml->value ) );
      }
      catch ( std::exception& ex ) {
        // don't care
      }
      //std::cout << "save vital data: " << xml->currentTime << " " << xml->label << " (" << xml->uom << ") " << xml->value << std::endl;
    }
    else if ( WAVE_SEG == el ) {
      if ( !xml->outer.wavewarning ) {
        xml->outer.wavewarning = true;
        xml->outer.output( ) << "Warning: waveform data parsing is not yet implemented" << std::endl;
      }

      //      bool added = false;
      //      auto& wave = xml->signals->addWave( xml->label, &added );
      //      if ( added ) {
      //        if ( !xml->uom.empty( ) ) {
      //          wave->setUom( xml->uom );
      //        }
      //        wave->setChunkIntervalAndSampleRate( 1024, 1024 / std::stoi( xml->sampleperiod ) );
      //      }
      //
      //      std::string vals;
      //      std::string_view view( xml->value );
      //      for ( size_t i = 0; i < xml->value.size( ); i += 4 ) {
      //        int val = std::stoi( xml->value.substr( i, 2 ), nullptr, 16 );
      //
      //        if ( !vals.empty( ) ) {
      //          vals.append( "," );
      //        }
      //        vals.append( std::to_string( val ) );
      //      }
      //
      //      wave->add( DataRow( xml->currentTime, vals ) );
      //      //std::cout << "save wave data: " << xml->currentTime << " " << xml->label << " (" << xml->sampleperiod << ") " << xml->value << std::endl;
    }
    else if ( 0 == strcmp( "PatientId", el ) ) {
      xml->currentPatientId = xml->currentText;
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
      if ( 0 == strcmp( "Time", el ) ) {

        xml->currentTime = xml->outer.parseTime( xml->currentText );
      }
      xml->state = ParseState::UNINTERESTING;
    }
  }

  void StpPhilipsReader::DataParser::chars( void * data, const char * text, int len ) {
    return PatientParser::chars( data, text, len );
  }
}

