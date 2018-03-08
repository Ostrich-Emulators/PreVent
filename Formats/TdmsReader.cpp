/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "TdmsReader.h"
#include "DataRow.h"
#include "SignalData.h"

#include <cmath>
#include <sstream>
#include <iostream>
#include <sys/stat.h>

#include <TdmsParser.h>
#include <TdmsChannel.h>
#include <TdmsGroup.h>
#include <TdmsMetaData.h>

TdmsReader::TdmsReader( ) : Reader( "TDMS" ) {
}

TdmsReader::~TdmsReader( ) {

}

dr_time TdmsReader::parsetime( const std::string& timestr ) {
  // sample: 14.12.2017 17:49:24,0.000000
  tm brokenTime;
  strptime( timestr.c_str( ), "%d.%m.%Y %H:%M:%S", &brokenTime );
  time_t sinceEpoch = timegm( &brokenTime );
  return sinceEpoch* 1000;
}

int TdmsReader::prepare( const std::string& recordset, SignalSet& info ) {
  int rslt = Reader::prepare( recordset, info );
  if ( 0 != rslt ) {
    return rslt;
  }

  parser.reset( new TdmsParser( recordset ) );
  return (parser->fileOpeningError( ) ? 1 : 0 );
}

void TdmsReader::finish( ) {
}

ReadResult TdmsReader::fill( SignalSet& info, const ReadResult& ) {
  int retcode = 0;
  parser->read( false );

  unsigned int groupCount = parser->getGroupCount( );
  for ( unsigned int i = 0; i < groupCount; i++ ) {
    TdmsGroup *group = parser->getGroup( i );
    if ( !group ) {
      continue;
    }

    unsigned int channelsCount = group->getGroupSize( );
    for ( unsigned int j = 0; j < channelsCount; j++ ) {
      TdmsChannel *ch = group->getChannel( j );
      if ( ch ) {

        unsigned int dataCount = ch->getDataCount( );
        unsigned int stringCount = ch->getStringCount( );
        // unsigned int dataSize = ( dataCount > 0 ) ? dataCount : stringCount;

        if ( dataCount ) {
          std::string name = ch->getName( );
          name = name.substr( 2, name.length( ) - 3 );
          output( ) << "reading " << name << std::endl;
          dr_time time = 0;
          double timeinc = 1;
          int freq = 0; // waves have an integer frequency
          auto propmap = ch->getProperties( );
          if ( propmap.count( "wf_increment" ) > 0 ) {
            timeinc = std::stod( propmap.at( "wf_increment" ) );
          }

          // figure out if this is a wave or a vital
          std::unique_ptr<SignalData>& signal = ( timeinc < 1.024
                ? info.addWave( name )
                : info.addVital( name ) );

          string unit = ch->getUnit( );
          if ( !unit.empty( ) ) {
            signal->setUom( unit );
          }

          for ( auto& p : propmap ) {
            // output( ) << p.first << " => " << p.second << std::endl;

            if ( "Unit_String" == p.first ) {
              signal->setUom( p.second );
            }
            else if ( "wf_starttime" == p.first ) {
              time = parsetime( p.second );
            }
            else if ( "wf_increment" == p.first ) {
              // already handled above
            }
            else if ( "Frequency" == p.first ) {
              double f = std::stod( p.second );
              signal->metad( )[SignalData::HERTZ] = f;
              signal->setValuesPerDataRow((int)f);
              if ( f > 1 ) { // wave!
                freq = std::stoi( p.second );
              }
            }
            else {
              signal->metas( )[p.first] = p.second;
            }
          }

          unsigned int type = ch->getDataType( );
          std::vector<double> data = ch->getDataVector( );
          if ( type == TdmsChannel::tdsTypeComplexSingleFloat
                || type == TdmsChannel::tdsTypeComplexDoubleFloat ) {
            std::cerr << "WARNING: complex data types are not yet supported"
                  << std::endl;
            //            std::vector<double> imData = ch->getImaginaryDataVector( );
            //            double iVal1 = imData.front( ), iVal2 = imData.back( );
            //            std::string fmt = ":\n\t%g";
            //            fmt.append( ( iVal1 < 0 ) ? "-i*%g ... %g" : "+i*%g ... %g" );
            //            fmt.append( ( iVal2 < 0 ) ? "-i*%g\n" : "+i*%g\n" );
            //            printf( fmt.c_str( ), data.front( ), fabs( iVal1 ), data.back( ), fabs( iVal2 ) );
          }
          else {
            if ( signal->wave( ) ) {
              // for waves, we need to construct a string of values that is 
              // {Frequency} items big
              //signal->setScale( 1000 ); // TDMS readouts seem to have 3 decimals

              std::stringstream vals;
              int cnt = 0;
              for ( auto& d : data ) {
                bool nan = isnan( d );

                if ( cnt == freq ) {
                  signal->add( DataRow( time++, vals.str( ) ) );
                  vals.clear( );
                  vals.str( std::string( ) );
                  cnt = 0;
                }

                if ( 0 != cnt ) {
                  vals << ",";
                }

                if ( nan ) {
                  vals << MISSING_VALUESTR;
                }
                else {
                  vals << (int) ( d * 1000 );
                }
                cnt++;
              }

              if ( 0 != cnt ) {
                // uh oh...we have some un-flushed data (probably an interrupted
                // read) so normalize it, then add it
                for ( cnt; cnt < freq; cnt++ ) {
                  vals << "," << MISSING_VALUESTR;
                }
                signal->add( DataRow( time++, vals.str( ) ) );
              }
            }
            else {
              // vitals are much easier...
              for ( auto& d : data ) {
                if ( !isnan( d ) ) {
                  // check if our number ends in .000000...
                  int dd = int(d );
                  bool isint = ( 0 == ( d - dd ) );
                  if ( isint ) {
                    signal->add( DataRow( time, std::to_string( dd ) ) );
                  }
                  else {
                    signal->add( DataRow( time, std::to_string( d ) ) );
                  }
                }
                time += timeinc;
              }
            }
          }
        }
        else if ( stringCount ) {
          std::cerr << "WARNING: string datasets are not yet supported" << std::endl;
          //          std::vector<std::string> strings = ch->getStringVector( );
          //          string str1 = strings.front( );
          //          if ( str1.empty( ) ){
          //            str1 = "empty string";
          //          }
          //          string str2 = strings.back( );
          //          if ( str2.empty( ) ){
          //            str2 = "empty string";
          //          }
          //          printf( ":\n\t%s ... %s\n", str1.c_str( ), str2.c_str( ) );
          //        }
          //        else{
          //          printf( ".\n" );
        }

        ch->freeMemory( );
      }
    }
  }

  // doesn't look like Tdms can do incremental reads, so we're at the end
  return ( 0 <= retcode ? ReadResult::END_OF_FILE : ReadResult::ERROR );
}

size_t TdmsReader::getSize( const std::string & input ) const {
  struct stat info;

  if ( stat( input.c_str( ), &info ) < 0 ) {
    perror( input.c_str( ) );
    return 0;
  }

  return info.st_size;

}