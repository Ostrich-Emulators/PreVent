/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "UmWfdbReader.h"
#include "DataRow.h"
#include "SignalData.h"
#include "SignalUtils.h"

#include <iostream>
#include <iterator>

namespace FormatConverter{
  const size_t UmWfdbReader::FIRST_VITAL_COL = 4;
  const size_t UmWfdbReader::TIME_COL = 1;
  const size_t UmWfdbReader::DATE_COL = 0;

  UmWfdbReader::UmWfdbReader( ) : WfdbReader( "UM WFDB" ) { }

  UmWfdbReader::~UmWfdbReader( ) { }

  int UmWfdbReader::prepare( const std::string& headerfile, std::unique_ptr<SignalSet>& info ) {
    int rslt = WfdbReader::prepare( headerfile, info );
    if ( 0 != rslt ) {
      return rslt;
    }

    std::string recordset( headerfile.substr( 0, headerfile.size( ) - 4 ) );
    // recordset should be a directory containing a .hea file, a .numerics.csv
    // file, and optionally a .clock.txt file
    std::string clockfile( recordset );
    clockfile.append( ".clock.txt" );
    std::ifstream clock( clockfile );
    if ( clock.good( ) ) {
      std::string firstline;
      std::getline( clock, firstline );

      std::string timepart( firstline.substr( 2, 19 ) );
      struct tm timeinfo;
      Reader::strptime2( timepart, "%m/%d/%Y %T %z", &timeinfo );

      std::string mspart( firstline.substr( 22, 3 ) );
      int ms = std::stoi( mspart );

      int tzoff = std::stoi( firstline.substr( 26, 3 ) );
      timeinfo.tm_gmtoff = tzoff * 3600;
      // FIXME: we still need to handle DST properly
      if ( timeinfo.tm_mon > 2 && timeinfo.tm_mon < 10 ) {
        timeinfo.tm_isdst = true;
      }
      time_t tt = std::mktime( &timeinfo );
      setBaseTime( tt * 1000 + ms );
    }

    // read the vitals out of the CSV, too
    std::string numsfile( recordset );
    numsfile.append( ".numerics.csv" );

    numerics.open( numsfile );
    if ( !numerics.good( ) ) {
      return -1;
    }

    std::string firstline;
    std::getline( numerics, firstline );
    headings = UmWfdbReader::splitcsv( firstline );

    std::ifstream infofile( recordset + ".info" );
    // if we have an info file, parse it and set the metadata in the SignalData
    while ( infofile.good( ) ) {
      std::getline( infofile, firstline );
      std::vector<std::string> parts = UmWfdbReader::splitcsv( firstline, ' ' );
      if ( !( parts.empty( ) || "#" == parts[0] || "dummy" == parts[0] ) ) {
        auto name = parts[4].substr( 1, parts[4].length( ) - 2 );
        auto& signal = info->addWave( name );
        for ( size_t pos = 5; pos < parts.size( ); pos++ ) {
          auto part = parts[pos];
          size_t eqpos = part.find( "=" );
          if ( eqpos != std::string::npos ) {
            auto key = part.substr( 0, eqpos );
            auto val = part.substr( eqpos + 1 );

            if ( "offset" == key ) {
              signal->setMeta( "offset", std::stod( val ) );
            }
            else if ( "anno_file" == key ) {
              std::ifstream annofile( val );
              std::string line;
              if ( annofile.is_open( ) ) {
                while ( std::getline( annofile, line ) ) {
                  std::vector<std::string> anns = UmWfdbReader::splitcsv( line, ' ' );
                  info->addAuxillaryData( val, FormatConverter::SignalSet::AuxData( std::stol( anns[0] ), SignalUtils::trim( anns[1] ) ) );
                }
              }
            }
          }
        }
      }
    }

    return rslt;
  }

  ReadResult UmWfdbReader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& lastrr ) {
    dr_time lastcsvtime = 0;
    ReadResult rslt = ReadResult::NORMAL;
    std::string csvline;

    // we have two different files and ways of reading
    // so, read from the CSV until we either exhaust the file
    // or get to a rollover. Once we've gotten there, read the
    // WFDB files for that day/file.

    while ( !numerics.eof( ) ) {
      auto offset = numerics.tellg( ); // in case we need to rewind the stream
      std::getline( numerics, csvline );
      if ( !csvline.empty( ) ) { // skip empty lines (expect maybe a trailing newline)
        dr_time csvtime;
        auto vitvals = linevalues( csvline, csvtime );

        if ( lastcsvtime > 0 && isRollover( lastcsvtime, csvtime ) ) {
          // "unread" the last line for the next call to fill()
          numerics.seekg( offset );
          rslt = ReadResult::END_OF_DAY;
          break;
        }
        lastcsvtime = csvtime;

        for ( auto& m : vitvals ) {
          bool added = false;
          auto& signal = info->addVital( m.first, &added );
          if ( added ) {
            signal->setChunkIntervalAndSampleRate( interval, 1 );
          }
          signal->add( DataRow( csvtime, m.second ) );
        }
      }
    }

    ReadResult rr = ( numerics.eof( ) || ReadResult::END_OF_DAY == rslt || ReadResult::NORMAL == rslt
        ? WfdbReader::fill( info, lastrr )
        : ReadResult::ERROR );
    return rr;
  }

  std::vector<std::string> UmWfdbReader::splitcsv( const std::string& csvline, char delim ) {
    std::vector<std::string> rslt;
    std::stringstream ss( csvline );
    std::string cellvalue;

    while ( std::getline( ss, cellvalue, delim ) ) {
      cellvalue.erase( std::remove( cellvalue.begin( ), cellvalue.end( ), '\r' ), cellvalue.end( ) );
      rslt.push_back( SignalUtils::trim( cellvalue ) );
    }
    return rslt;
  }

  std::map<std::string, std::string> UmWfdbReader::linevalues( const std::string& csvline, dr_time& timer ) {
    std::vector<std::string> strings = splitcsv( csvline );
    std::map<std::string, std::string> values;

    struct tm timeinfo = { 0 };
    std::string date = strings[DATE_COL];
    std::string time = strings[TIME_COL];

    // fix the date string by adding leading zeros to the month, day
    if ( '/' == date[1] ) {
      // need a leading 0 for the month
      date.insert( date.begin( ), '0' );
    }
    // do we need a leading zero for the day, too?
    if ( '/' == date[4] ) {
      date.insert( date.begin( ) + 3, '0' );
    }
    Reader::strptime2( date, "%m/%d/%Y", &timeinfo );
    Reader::strptime2( time, "%H:%M:%S", &timeinfo );

    // FIXME: get ms, too
    int ms = std::stoi( time.substr( 9, 3 ) );

    timer = modtime( timegm( &timeinfo ) * 1000 + ms );

    for ( size_t i = FIRST_VITAL_COL; i < headings.size( ); i++ ) {
      const auto& h = headings[i];
      const auto& v = strings[i];
      if ( v.size( ) > 0 ) {
        values[h] = v;
      }
    }
    return values;
  }
}