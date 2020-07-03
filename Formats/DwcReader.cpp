/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "DwcReader.h"
#include "DataRow.h"
#include "SignalData.h"
#include "SignalUtils.h"

#include <filesystem>
#include <iostream>
#include <iterator>

namespace FormatConverter{
  const size_t DwcReader::FIRST_VITAL_COL = 4;
  const size_t DwcReader::TIME_COL = 1;
  const size_t DwcReader::DATE_COL = 0;

  DwcReader::DwcReader( ) : WfdbReader( "DWC" ) { }

  DwcReader::~DwcReader( ) { }

  dr_time DwcReader::converttime( const std::string& timeline ) {
    struct tm timeinfo;
    std::vector<std::string> parts = SignalUtils::splitcsv( timeline, ' ' );

    std::string datepart( parts[0] );
    std::string timepart( parts[1] );
    std::string tzpart( parts[2] );

    if ( '/' == datepart[1] ) {
      // need a leading 0 for the month
      datepart.insert( datepart.begin( ), '0' );
    }
    // do we need a leading zero for the day, too?
    if ( '/' == datepart[4] ) {
      datepart.insert( datepart.begin( ) + 3, '0' );
    }

    std::vector<std::string> timepieces = SignalUtils::splitcsv( timepart, '.' );
    timepart = timepieces[0];
    int ms = std::stoi( timepieces[1] );


    Reader::strptime2( datepart + " " + timepart, "%m/%d/%Y %T %z", &timeinfo );

    int tzoff = std::stoi( tzpart.substr( 0, 3 ) );
    timeinfo.tm_gmtoff = tzoff * 3600;
    // FIXME: we still need to handle DST properly
    if ( timeinfo.tm_mon > 2 && timeinfo.tm_mon < 10 ) {
      timeinfo.tm_isdst = true;
    }
    time_t tt = std::mktime( &timeinfo );
    return (tt * 1000 + ms );
  }

  int DwcReader::prepare( const std::string& infoname, SignalSet * info ) {
    auto fspath = std::filesystem::path{ infoname };
    auto recordset = fspath.replace_extension( ".hea" );
    auto basename = recordset.parent_path( ).append( recordset.stem( ).string( ) ).string( );
    int rslt = WfdbReader::prepare( recordset.string( ), info );
    if ( 0 != rslt ) {
      return rslt;
    }

    // recordset should be a directory containing a .hea file, a .numerics.csv
    // file, and optionally a .clock.txt file
    auto clockfile = basename + ".clock.txt";
    clocktimes = SignalUtils::loadAuxData( clockfile );
    if ( !clocktimes.empty( ) ) {
      dr_time basetime = converttime( clocktimes[0].data );
      setBaseTime( basetime );

      clocktimes.erase( clocktimes.begin( ) );

      std::for_each( clocktimes.begin( ), clocktimes.end( ), [&basetime]( TimedData & td ) {
        td.time += basetime;
      } );
    }

    // read the vitals out of the CSV, too
    auto numsfile = basename + ".numerics.csv";
    numerics.open( numsfile );
    if ( !numerics.good( ) ) {
      std::cerr << "no numerics file found" << std::endl;
      return -1;
    }

    std::string firstline;
    std::getline( numerics, firstline );
    headings = SignalUtils::splitcsv( firstline );

    std::ifstream infofile( infoname );
    // if we have an info file, parse it and set the metadata in the SignalData
    while ( infofile.good( ) ) {
      std::getline( infofile, firstline );
      std::vector<std::string> parts = SignalUtils::splitcsv( firstline, ' ' );
      if ( !( parts.empty( ) || "#" == parts[0] || "dummy" == parts[0] ) ) {
        auto name = parts[4].substr( 1, parts[4].length( ) - 2 );
        auto signal = info->addWave( name );
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
              dr_time offsetter = basetime( );
              for ( auto& a : SignalUtils::loadAuxData( val ) ) {

                a.time += offsetter;
                annomap[name][val].push_back( a );
              }
            }
          }
        }
      }
    }

    return rslt;
  }

  ReadResult DwcReader::fill( SignalSet * info, const ReadResult & lastrr ) {
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
          auto signal = info->addVital( m.first, &added );
          if ( added ) {
            signal->setChunkIntervalAndSampleRate( interval, 1 );
          }
          signal->add( DataRow::from( csvtime, m.second ) );
        }
      }
    }

    ReadResult rr = rslt;
    if ( numerics.eof( ) ) {
      rr = ReadResult::END_OF_FILE;
    }

    if ( !this->skipwaves( ) || ReadResult::ERROR == rslt ) {
      rr = WfdbReader::fill( info, lastrr );
    }

    dr_time last = ( ReadResult::END_OF_FILE == rr
        ? std::numeric_limits<long>::max( )
        : info->latest( ) );
    if ( ReadResult::ERROR != rr ) {
      size_t cnt = 0;
      for ( auto& td : clocktimes ) {
        if ( td.time <= last ) {
          cnt++;
          info->addAuxillaryData( "Wall_Times", td );
        }
      }
      clocktimes.erase( clocktimes.begin( ), clocktimes.begin( ) + cnt );

      for ( auto& map : annomap ) {
        const auto name = map.first;
        auto wave = info->addWave( name );
        for ( auto& map : map.second ) {
          size_t cnt = 0;
          for ( auto& td : map.second ) {
            if ( td.time <= last ) {

              cnt++;
              wave->addAuxillaryData( map.first, td );
            }
          }
          map.second.erase( map.second.begin( ), map.second.begin( ) + cnt );
        }
      }
    }

    return rr;
  }

  std::map<std::string, std::string> DwcReader::linevalues( const std::string& csvline, dr_time & timer ) {
    std::vector<std::string> strings = SignalUtils::splitcsv( csvline );
    std::map<std::string, std::string> values;

    std::string timestr = strings[DATE_COL] + " " + strings[TIME_COL];
    timer = modtime( converttime( timestr ) );
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