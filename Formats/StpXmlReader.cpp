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

const std::string StpXmlReader::MISSING_VALUESTR( "-32768" );
const std::set<std::string> StpXmlReader::Hz60({ "RR", "VNT_PRES", "VNT_FLOW" } );
const std::set<std::string> StpXmlReader::Hz120({ "ICP1", "ICP2", "ICP4", "LA4" } );
std::string StpXmlReader::working;
const int StpXmlReader::INHEADER = 1;
const int StpXmlReader::INVITAL = 2;
const int StpXmlReader::INWAVE = 4;
const int StpXmlReader::INNAME = 8;

StpXmlReader::StpXmlReader( ) : warnMissingName( true ), warnJunkData( true ),
prevtime( 0 ), currvstime( 0 ), lastvstime( 0 ), currwavetime( 0 ), lastwavetime( 0 ) {
  LIBXML_TEST_VERSION
}

StpXmlReader::StpXmlReader( const StpXmlReader& orig ) {
}

StpXmlReader::~StpXmlReader( ) {
}

void StpXmlReader::finish( ) {
  XML_ParserFree( parser );
}

size_t StpXmlReader::getSize( const std::string& input ) const {
  struct stat info;

  if ( stat( input.c_str( ), &info ) < 0 ) {
    perror( input.c_str( ) );
    return 0;
  }

  return info.st_size;
}

void StpXmlReader::start( void * data, const char * el, const char ** attr ) {
  std::map<std::string, std::string> attrs;
  for ( int i = 0; attr[i]; i += 2 ) {
    attrs[attr[i]] = attr[i + 1];
  }

  StpXmlReader * rdr = static_cast<StpXmlReader *> ( data );
  rdr->start( el, attrs );
}

void StpXmlReader::end( void * data, const char * el ) {
  StpXmlReader * rdr = static_cast<StpXmlReader *> ( data );
  rdr->end( el, trim( working ) );
  working.clear( );
}

void StpXmlReader::chars( void * data, const char * text, int len ) {
  working.append( text, len );
}

void StpXmlReader::setstate( int st ) {
  state = st;
}

void StpXmlReader::start( const std::string& element, std::map<std::string, std::string>& attrs ) {
  if ( "FileInfo" == element ) {
    setstate( INHEADER );
  }
  else if ( "VitalSigns" == element ) {
    setstate( INVITAL );
    lastvstime = currvstime;
    currvstime = time( attrs["Time"] );
    if ( isRollover( lastvstime, currvstime ) ) {
      rslt = ReadResult::END_OF_DAY;
      prevtime = currvstime;
      filler = &saved;
    }
  }
  else if ( "Waveforms" == element ) {
    setstate( INWAVE );
    lastwavetime = currwavetime;
    currwavetime = time( attrs["Time"] );
    if ( isRollover( lastwavetime, currwavetime ) ) {
      rslt = ReadResult::END_OF_DAY;
      prevtime = currwavetime;
      filler = &saved;
    }
  }
  else if ( "PatientName" == element ) {
    setstate( INNAME );
  }

  if ( "Value" == element ) {
    if ( 0 != attrs.count( "UOM" ) ) {
      uom = attrs["UOM"];
    }
    if ( 0 != attrs.count( "Q" ) ) {
      q = attrs["Q"];
    }
  }

  std::cout << "start " << element << std::endl;
  if ( !attrs.empty( ) ) {
    for ( auto& m : attrs ) {
      std::cout << "\t" << m.first << "=>" << m.second << std::endl;
    }
  }
}

void StpXmlReader::end( const std::string& element, const std::string& text ) {
  if ( INHEADER == state ) {
    filler->metadata( )[element] = text;
    rslt = ReadResult::NORMAL;
  }
  else if ( INNAME == state ) {
    if ( 0 == prevtime ) {
      filler->metadata( )["Patient Name"] = text;
    }
    else {
      std::string pname = filler->metadata( )["Patient Name"];
      if ( text != pname ) {
        rslt = ReadResult::END_OF_PATIENT;
        // we've cut over to a new set of data, so
        // save the data we parse now to a different SignalSet
        filler = &saved;
      }
    }
  }
  else if ( INVITAL == state ) {
    if ( "Par" == element ) {
      label = text;
    }
    else if ( "Value" == element ) {
      value = text;
    }
    else if ( "VS" == element ) {
      bool added = false;
      std::unique_ptr<SignalData>& sig = filler->addVital( label, &added );

      if ( added ) {
        sig->metad( ).insert( std::make_pair( SignalData::HERTZ, 0.5 ) );
        sig->metas( ).insert( std::make_pair( SignalData::MSM, MISSING_VALUESTR ) );
        sig->metas( ).insert( std::make_pair( SignalData::TIMEZONE, "UTC" ) );

        if ( !uom.empty( ) ) {
          sig->setUom( uom );
        }
      }

      sig->add( DataRow( currvstime, value ) );
    }
  }
  else if ( INWAVE == state ) {

  }



  std::cout << "end " << element << std::endl;//"(" << text << ")" << std::endl;
}

int StpXmlReader::prepare( const std::string& fname, SignalSet& info ) {
  int rr = Reader::prepare( fname, info );
  if ( 0 == rr ) {
    parser = XML_ParserCreate( NULL );
    XML_SetUserData( parser, this );
    XML_SetElementHandler( parser, StpXmlReader::start, StpXmlReader::end );
    XML_SetCharacterDataHandler( parser, StpXmlReader::chars );
    input.open( fname, std::ifstream::in );
    rslt = ReadResult::NORMAL;
  }
  return rr;
}

void StpXmlReader::copysaved( SignalSet& newset ) {
  newset.metadata( ).insert( saved.metadata( ).begin( ), saved.metadata( ).end( ) );
  saved.metadata( ).clear( );

  for ( auto& m : saved.vitals( ) ) {
    std::unique_ptr<SignalData>& savedsignal = m.second;

    bool added = false;
    std::unique_ptr<SignalData>& infodata = newset.addVital( m.first, &added );

    if ( added ) {
      auto& smap = savedsignal->metas( );
      infodata->metas( ).insert( smap.begin( ), smap.end( ) );

      auto& dmap = savedsignal->metad( );
      infodata->metad( ).insert( dmap.begin( ), dmap.end( ) );

      auto& imap = savedsignal->metai( );
      infodata->metai( ).insert( imap.begin( ), imap.end( ) );
    }

    int rows = savedsignal->size( );
    for ( int row = 0; row < rows; row++ ) {
      const std::unique_ptr<DataRow>& datarow = savedsignal->pop( );
      infodata->add( *datarow );
    }
  }

  for ( auto& m : saved.waves( ) ) {
    std::unique_ptr<SignalData>& savedsignal = m.second;

    bool added = false;
    std::unique_ptr<SignalData>& infodata = newset.addWave( m.first, &added );

    if ( added ) {
      auto& smap = savedsignal->metas( );
      infodata->metas( ).insert( smap.begin( ), smap.end( ) );

      auto& dmap = savedsignal->metad( );
      infodata->metad( ).insert( dmap.begin( ), dmap.end( ) );

      auto& imap = savedsignal->metai( );
      infodata->metai( ).insert( imap.begin( ), imap.end( ) );
    }

    int rows = savedsignal->size( );
    for ( int row = 0; row < rows; row++ ) {
      const std::unique_ptr<DataRow>& datarow = savedsignal->pop( );
      infodata->add( *datarow );
    }
  }

  saved.vitals( ).clear( );
  saved.waves( ).clear( );
}

ReadResult StpXmlReader::fill( SignalSet & info, const ReadResult& lastfill ) {
  if ( ReadResult::END_OF_DAY == lastfill || ReadResult::END_OF_PATIENT == lastfill ) {
    copysaved( info );
  }
  filler = &info;

  const int buffsz = 16384;
  std::vector<char> buffer( buffsz, 0 );
  while ( input.read( buffer.data( ), buffer.size( ) ) ) {
    XML_Parse( parser, &buffer[0], buffer.size( ), input.gcount( ) < buffsz );
    if ( ReadResult::NORMAL != rslt ) {
      return rslt;
    }
  }

  return ReadResult::END_OF_FILE;
}

ReadResult StpXmlReader::processNode( SignalSet& info ) {
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
        //        savedmeta = getHeaders( );

        //        savedmeta.erase( "Size" ); // Size is meaningless for us
        //        info.metadata( ).insert( savedmeta.begin( ), savedmeta.end( ) );

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
          if ( xmlTextReaderIsEmptyElement( reader ) ) {
            std::cout << "no vitals data" << std::endl;
          }
          else if ( isRollover( true ) ) {
            return ReadResult::END_OF_DAY;
          }
          else {
            handleVitalsSet( info );
          }
        }
        else if ( "Waveforms" == ele ) {
          if ( xmlTextReaderIsEmptyElement( reader ) ) {
            std::cout << "no waveform data" << std::endl;
          }
          else if ( isRollover( false ) ) {
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
        && ( "VitalSigns" == element || "Waveforms" == element )
        && !xmlTextReaderIsEmptyElement( reader ) ) {
      // at level 2, it's either VitalSigns or Waveforms elements
      bool isvital = ( "VitalSigns" == element );

      if ( isRollover( isvital ) ) {
        return ReadResult::END_OF_DAY;
      }
      else {
        if ( isvital ) {
          if ( lastvstime == currvstime ) {
            std::cout << "skipping duplicate vital time: " << lastvstime << std::endl;
          }
          else {
            handleVitalsSet( info );
          }
        }
        else if ( "Waveforms" == element ) {
          if ( lastwavetime == currwavetime ) {
            std::cout << "skipping duplicate wave time: " << lastwavetime << std::endl;
          }
          else {
            handleWaveformSet( info );
          }
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

ReadResult StpXmlReader::handleSegmentPatientName( SignalSet& info ) {
  std::string patientname = textAndClose( );
  //savedmeta["Patient Name"] = patientname;

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

  // std::cout << "patient name is " << patientname << std::endl;
  return ReadResult::NORMAL;
}

bool StpXmlReader::isRollover( const time_t& then, const time_t& now ) const {
  if ( 0 != then ) {
    const int cdoy = gmtime( &now )->tm_yday;
    const int pdoy = gmtime( &then )->tm_yday;
    if ( cdoy != pdoy ) {
      return true;
    }
  }

  return false;
}

time_t StpXmlReader::time( const std::string& timer ) const {
  if ( std::string::npos == timer.find( " " ) ) {
    return std::stol( timer );
  }

  // we have a local time that we need to convert
  tm mytime;
  strptime( timer.c_str( ), "%m/%d/%Y %I:%M:%S %p", &mytime );
  // now convert our local time to UTC
  time_t local = mktime( &mytime );
  mytime = *gmtime( &local );
  return mktime( &mytime );
}

bool StpXmlReader::isRollover( bool forVitals ) {
  // check the time against our previous time
  std::map<std::string, std::string> map = getAttrs( );

  time_t currtime;
  std::string timer( map["Time"] );
  if ( std::string::npos == timer.find( " " ) ) {
    currtime = std::stol( timer );
  }
  else {
    // we have a local time that we need to convert
    tm mytime;
    strptime( timer.c_str( ), "%m/%d/%Y %I:%M:%S %p", &mytime );
    // now convert our local time to UTC
    time_t local = mktime( &mytime );
    mytime = *gmtime( &local );
    currtime = mktime( &mytime );
  }

  if ( forVitals ) {
    lastvstime = currvstime;
    currvstime = currtime;
  }
  else {
    lastwavetime = currwavetime;
    currwavetime = currtime;
  }


  if ( 0 == prevtime ) {
    prevtime = currtime;
  }
  else {
    const int cdoy = gmtime( &currtime )->tm_yday;
    const int pdoy = gmtime( &prevtime )->tm_yday;
    if ( cdoy != pdoy ) {
      prevtime = currtime;
      return true;
    }
  }

  return false;
}

void StpXmlReader::handleWaveformSet( SignalSet& info ) {
  next( );
  std::string element = stringAndFree( xmlTextReaderName( reader ) );
  while ( "WaveformData" == element ) {
    std::map<std::string, std::string> map;
    std::string vals = textAndAttrsToClose( map );

    const std::string& wave = map["Channel"];
    if ( wave.empty( ) ) {
      if ( warnMissingName ) {
        std::cerr << "ignoring unnamed waveforms" << std::endl;
        warnMissingName = false;
      }
    }
    else {
      if ( shouldExtract( wave ) ) {
        // reverse the oversampling (if any) and set the true Hz
        int thz = 240;
        if ( 0 != Hz60.count( wave ) ) {
          vals = resample( vals, 60 );
          thz = 60;
        }
        else if ( 0 != Hz120.count( wave ) ) {
          vals = resample( vals, 120 );
          thz = 120;
        }
        const int hz = thz;

        if ( waveIsOk( vals ) ) {
          bool first;
          std::unique_ptr<SignalData>& sig = info.addWave( wave, &first );
          if ( first ) {
            sig->metas( ).insert( std::make_pair( SignalData::MSM, MISSING_VALUESTR ) );
            sig->metas( ).insert( std::make_pair( SignalData::TIMEZONE, "UTC" ) );

            if ( 0 != map.count( "UOM" ) ) {
              sig->setUom( map["UOM"] );
            }
          }

          sig->metad( )[SignalData::HERTZ] = hz;
          sig->add( DataRow( currwavetime, vals ) );
        }
        else if ( warnJunkData ) {
          warnJunkData = false;
          std::cerr << "skipping waveforms with no usable data" << std::endl;
        }
      }
    }
    next( );
    element = stringAndFree( xmlTextReaderName( reader ) );
  }
}

void StpXmlReader::handleVitalsSet( SignalSet & info ) {
  next( );
  std::string element = stringAndFree( xmlTextReaderName( reader ) );
  while ( "VS" == element ) {
    std::string param;
    std::string uom;
    DataRow row = handleOneVs( param, uom );

    if ( shouldExtract( param ) ) {
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
    }
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

  return DataRow( currvstime, val, hi, lo );
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

std::string StpXmlReader::trim( std::string & totrim ) {
  // ltrim
  totrim.erase( totrim.begin( ), std::find_if( totrim.begin( ), totrim.end( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );

  // rtrim
  totrim.erase( std::find_if( totrim.rbegin( ), totrim.rend( ),
      std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), totrim.end( ) );

  return totrim;
}

std::string StpXmlReader::text( ) {
  next( );
  std::string value = stringAndFree( xmlTextReaderValue( reader ) );

  return trim( value );
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

std::string StpXmlReader::textAndAttrsToClose( std::map<std::string, std::string>& attrs ) {
  if ( xmlReaderTypes::XML_READER_TYPE_ELEMENT == xmlTextReaderNodeType( reader ) ) {
    attrs = getAttrs( );
  }
  else {
    std::cerr << "cannot read attributes from non-opening element" << std::endl;
  }

  return textAndClose( );
}

std::string StpXmlReader::stringAndFree( xmlChar * chars ) const {
  std::string ret;
  if ( NULL != chars ) {
    ret = std::string( (char *) chars );
    xmlFree( chars );
  }
  return ret;
}

int StpXmlReader::next( ) {
  int ret = xmlTextReaderRead( reader );
  if ( ret != 1 ) {
    if ( ret < 0 ) {
      std::cerr << "error reading stream!" << std::endl;
    }
    return ret; // FIXME: this is confusing for the caller
  }

  int type = xmlTextReaderNodeType( reader );

  if ( xmlReaderTypes::XML_READER_TYPE_SIGNIFICANT_WHITESPACE == type ) {
    //std::cout << "skipping whitespace" << std::endl;
    return next( );
  }

  //  int depth = xmlTextReaderDepth( reader );
  //  std::cout << depth << ":";
  //  for ( int i = 0; i < depth; i++ ) {
  //    std::cout << "  ";
  //  }
  //
  //  std::map<int, std::string> typelkp;
  //  typelkp[xmlReaderTypes::XML_READER_TYPE_ELEMENT] = "open";
  //  typelkp[xmlReaderTypes::XML_READER_TYPE_END_ELEMENT] = "close";
  //  typelkp[xmlReaderTypes::XML_READER_TYPE_SIGNIFICANT_WHITESPACE] = "sig white";
  //  typelkp[xmlReaderTypes::XML_READER_TYPE_TEXT] = "text";
  //  typelkp[xmlReaderTypes::XML_READER_TYPE_ATTRIBUTE] = "attr";
  //  // typelkp[xmlReaderTypes] = "";
  //
  //  std::cout << stringAndFree( xmlTextReaderName( reader ) )
  //      << ": ->" << stringAndFree( xmlTextReaderValue( reader ) ) << "<-"
  //      << "(" << ( 0 == typelkp.count( type )
  //      ? "type " + std::to_string( type )
  //      : typelkp[type] )
  //      << ")" << std::endl;
  return type;
}

std::string StpXmlReader::nextelement( ) {
  // ignore everything until the next start element
  while ( xmlReaderTypes::XML_READER_TYPE_ELEMENT != next( ) ) {
    // nothing to do in the loop...the next() does it all
  }

  return stringAndFree( xmlTextReaderName( reader ) );
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
  bool oneok = false;
  std::stringstream stream( wavedata );
  for ( std::string each; std::getline( stream, each, ',' ); ) {
    if ( !( "-32768" == each || "-32753" == each ) ) {
      return true;
    }
  }
  return false;
}
