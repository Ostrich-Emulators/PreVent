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
#include "Log.h"

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

  const std::string StpPhilipsReader::XML_HEADER = "<?xml ";
  const std::string StpPhilipsReader::LT = "<";
  const std::string StpPhilipsReader::GT = ">";
  const std::string StpPhilipsReader::PATIENT_NAME = "Patient Name";
  const std::string StpPhilipsReader::PATIENT_MRN = "MRN";
  const std::string StpPhilipsReader::PATIENT_PRIMARYID = "PrimaryId";
  const std::string StpPhilipsReader::PATIENT_LIFETIMEID = "LifetimeId";
  const std::string StpPhilipsReader::PATIENT_DOB = "DOB";
  const std::string StpPhilipsReader::PATIENT_DATEOFBIRTH = "Date of Birth";

  StpPhilipsReader::StpPhilipsReader( const std::string& name ) : StpReaderBase( name ) { }

  StpPhilipsReader::StpPhilipsReader( const StpPhilipsReader& orig ) : StpReaderBase( orig ) { }

  StpPhilipsReader::~StpPhilipsReader( ) { }

  std::string_view StpPhilipsReader::peekPatientId( const std::string& xmldoc, bool ispatientdoc ) {
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

    std::string_view sv( xmldoc );
    sv.remove_suffix( xmldoc.size( ) - end );
    sv.remove_prefix( start );
    return sv;
  }

  dr_time StpPhilipsReader::peekTime( const std::string_view& xmldoc ) {
    size_t start = xmldoc.find( "<Time>" ) + 6;
    size_t end = xmldoc.find( "</Time>", start );

    return ( std::string::npos == start || std::string::npos == end )
        ? 0
        : parseTime( std::string( xmldoc.substr( start, ( end - start ) ) ) );
  }

  int StpPhilipsReader::prepare( const std::string& filename, SignalSet * data ) {
    int rslt = StpReaderBase::prepare( filename, data );
    wavewarning = false;
    return rslt;
  }

  ReadResult StpPhilipsReader::fill( SignalSet * info, const ReadResult& lastrr ) {
    currentTime = 0;
    Log::debug( ) << "initial reading from input stream (popped:" << work.popped( ) << ")" << std::endl;

    if ( ReadResult::END_OF_PATIENT == lastrr ) {
      info->setMeta( PATIENT_NAME, "" ); // reset the patient name
      info->setMeta( PATIENT_PRIMARYID, "" );
      info->setMeta( PATIENT_LIFETIMEID, "" );
      info->setMeta( PATIENT_DOB, "" );
      info->setMeta( PATIENT_DATEOFBIRTH, "" );
    }

    int cnt = StpReaderBase::readMore( );
    if ( cnt < 0 ) {
      return ReadResult::ERROR;
    }

    std::string xmldoc;
    std::string rootelement;
    std::string patientId;
    //    int loopcounter = 0;
    while ( cnt > 0 ) {
      //      if ( 0 == loopcounter % 3000 ) {
      //        std::cout << "(loop " << loopcounter << ") popped: " << work.popped( ) << "; work buffer size: " << work.size( ) << "; available:" << work.available( ) << std::endl;
      //      }
      //      loopcounter++;

      if ( work.available( ) < 1024 * 768 ) {
        // we should never come close to filling up our work buffer
        // so if we have, make sure the user knows
        Log::error( ) << "work buffer is too full...something is going wrong" << std::endl;
        return ReadResult::ERROR;
      }

      // read as many segments as we can before reading more data
      ChunkReadResult rslt = ChunkReadResult::OK;

      size_t beforecheck = work.popped( );
      while ( hasCompleteXmlDoc( xmldoc, rootelement ) && ChunkReadResult::OK == rslt ) {
        //output( ) << "next segment is " << std::dec << segsize << " bytes big" << std::endl;
        //        if ( 2213001668L <= work.popped( ) ) {
        //          std::cout << "get ready: " << work.popped( ) << " " << work.size( ) << std::endl;
        //        }

        bool isPatientDoc = ( "PatientUpdateResponse" == rootelement );

        // if the patient has changed, we need to end this read before processing
        // the XML fully. Same goes with the rollover check
        std::string_view newpatientid = peekPatientId( xmldoc, isPatientDoc );
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
        xmlpassthru passthru = { .signals = info, .state = ParseState::UNINTERESTING, .outer = *this };
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
          Log::error( ) << XML_ErrorString( err )
              << " line: " << XML_GetCurrentLineNumber( parser )
              << " column: " << XML_GetCurrentColumnNumber( parser )
              << std::endl;
          return ReadResult::ERROR;
        }

        XML_ParserFree( parser );

        beforecheck = work.popped( );
        //        if ( 2213001668L <= work.popped( ) ) {
        //          std::cout << "get ready: " << work.popped( ) << " " << work.size( ) << std::endl;
        //        }
      }

      cnt = readMore( );
      if ( cnt < 0 ) {
        return ReadResult::ERROR;
      }
    }

    Log::debug( ) << "file is exhausted" << std::endl;

    // copy any data we have left in our filler set to the real set

    // if we still have stuff in our work buffer, process it
    if ( !work.empty( ) ) {
      Log::warn( ) << "still have stuff in our work buffer!" << std::endl;
    }

    return ReadResult::END_OF_FILE;
  }

  void StpPhilipsReader::skipUntil( const std::string& needle ) {
    CircularBuffer<char> haystack( needle.size( ) );

    // fill our circular buffer to start
    for ( size_t i = 0; i < needle.size( ); i++ ) {
      haystack.push( (char) work.pop( ) );
    }

    // algorithm: walk through the characters in our work buffer
    // if we get a (character) match, advance
    // if we get a miss, roll the buffer, then go back to
    // checking starting at position 0 again

    size_t needlpos = 0;
    while ( needlpos < needle.size( ) ) {
      if ( needle[needlpos] == haystack.read( needlpos ) ) {
        needlpos++;
      }
      else {
        needlpos = 0;
        haystack.pop( );
        haystack.push( (char) work.pop( ) );
      }
    }

    // if we get here before running out of buffer, we've found our string
    // rewind to the beginning of the needle
    work.rewind( needle.size( ) );


    //    std::string_view view( needle );
    //    while ( !work.empty( ) ) {
    //      while ( work.pop( ) != view[0] ) {
    //        // nothing to do here, just wanted to pop
    //      }
    //
    //      // found the first character, so see if we found the whole string
    //      if ( view.substr( 1 ) == readString( needle.size( ) - 1 ) ) {
    //        // found!
    //        work.rewind( ); // rewind to the first character
    //        break;
    //      }
    //    }
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
      skipUntil( XML_HEADER );
      size_t xmlstart = work.popped( ) - markpop;

      work.skip( XML_HEADER.size( ) ); // skip "<?xml" but not any other attributes
      skipUntil( LT ); // find the root element
      size_t rootElementNameStart = work.popped( ) + 1;
      skipUntil( GT );
      size_t rootElementNameStop = work.popped( );

      size_t rootElementNameLength = ( rootElementNameStop - rootElementNameStart );
      work.rewind( rootElementNameLength );

      auto rootpieces = SignalUtils::splitcsv( popString( rootElementNameLength ), ' ' );
      rootelement.assign( rootpieces[0] );
      if ( 0 == elementClosings.count( rootelement ) ) {
        elementClosings[rootelement] = "</" + rootelement + ">";
      }

      const std::string& endneedle = elementClosings[rootelement];
      skipUntil( endneedle );
      work.skip( endneedle.size( ) );
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
      found.assign( temp );
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
    auto info = std::unique_ptr<SignalSet>{ std::make_unique<BasicSignalSet>( ) };
    StpPhilipsReader reader;
    reader.setNonbreaking( true );
    int failed = reader.prepare( input, info.get( ) );
    if ( failed ) {
      Log::error( ) << "error while opening input file. error code: " << failed << std::endl;
      return metas;
    }
    reader.setMetadataOnly( );

    ReadResult last = ReadResult::FIRST_READ;

    bool okToContinue = true;
    while ( okToContinue ) {
      last = reader.fill( info.get( ), last );
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
          Log::error( ) << "error while reading input file" << std::endl;
          okToContinue = false;
          break;
      }
    }

    reader.finish( );

    return metas;
  }

  StpReaderBase::StpMetadata StpPhilipsReader::metaFromSignalSet( const std::unique_ptr<SignalSet>& info ) {
    StpReaderBase::StpMetadata meta;
    if ( info->metadata( ).count( PATIENT_NAME ) > 0 ) {
      meta.name = info->metadata( ).at( PATIENT_NAME );
    }
    if ( info->metadata( ).count( PATIENT_MRN ) > 0 ) {
      meta.mrn = info->metadata( ).at( PATIENT_MRN );
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

    if ( std::string::npos == datetime.find( '.' ) ) {
      // no ms
      std::sscanf( datetime.c_str( ), "%d-%d-%dT%d:%d:%d%d", &y, &M, &d, &h, &m, &s, &tzh );
      ms = 0;
    }
    else {
      std::sscanf( datetime.c_str( ), "%d-%d-%dT%d:%d:%d.%dZ", &y, &M, &d, &h, &m, &s, &ms );
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
        xml->signals->setMeta( PATIENT_NAME, xml->currentText );
      }
      else if ( 0 == strcmp( "DateOfBirth", el ) ) {
        xml->signals->setMeta( PATIENT_DOB, std::to_string( xml->outer.parseTime( xml->currentText ) ) );
        xml->signals->setMeta( PATIENT_DATEOFBIRTH, xml->currentText );
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
    xmlpassthru * xml = (xmlpassthru*) data;
    xml->currentText.assign( text, len );
    //SignalUtils::trim( xml->currentText );
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
      auto vital = xml->signals->addVital( xml->label, &added );
      if ( added ) {
        vital->setUom( xml->uom );
        vital->setChunkIntervalAndSampleRate( 1024, 1 );
      }

      // we call stoi to ensure we have an actual value
      // ...if an exception is thrown, just move on
      try {
        vital->add( DataRow::one( xml->currentTime, xml->value ) );
      }
      catch ( std::exception& ex ) {
        // don't care
      }
      //std::cout << "save vital data: " << xml->currentTime << " " << xml->label << " (" << xml->uom << ") " << xml->value << std::endl;
    }
    else if ( WAVE_SEG == el ) {
      if ( !xml->outer.wavewarning ) {
        xml->outer.wavewarning = true;
        Log::warn( ) << "Warning: waveform data parsing is not yet implemented" << std::endl;
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

      // FIXME: check this->skipwaves()

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

