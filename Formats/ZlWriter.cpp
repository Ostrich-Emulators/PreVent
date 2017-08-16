/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "ZlWriter.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <limits>
#include <fstream>
#include <set>

#include "SignalSet.h"
#include "SignalData.h"

ZlWriter::ZlWriter( ) {
}

ZlWriter::ZlWriter( const ZlWriter& ) {
}

ZlWriter::~ZlWriter( ) {
}

int ZlWriter::initDataSet( const std::string& directory, const std::string& namestart,
    int ) {
  filestart = directory + namestart;
  return 0;
}

std::vector<std::string> ZlWriter::closeDataSet( ) {
  std::vector<std::string> vec;
  vec.push_back( filename );
  return vec;
}

int ZlWriter::drain( SignalSet& info ) {
  time_t firsttime = std::numeric_limits<time_t>::max( );

  std::unique_ptr<DataRow> vits[info.vitals( ).size( )];
  std::unique_ptr<DataRow> wavs[info.waves( ).size( )];
  std::vector<std::string> vls;
  std::vector<std::string> ws;

  int vidx = 0;
  for ( auto& map : info.vitals( ) ) {
    vls.push_back( map.first );
    vits[vidx++] = std::move( map.second->pop( ) );

    if ( map.second->startTime( ) < firsttime ) {
      firsttime = map.second->startTime( );
    }
  }

  int widx = 0;
  for ( auto& map : info.waves( ) ) {
    ws.push_back( map.first );
    wavs[widx++] = std::move( map.second->pop( ) );

    if ( map.second->startTime( ) < firsttime ) {
      firsttime = map.second->startTime( );
    }
  }

  char recsuffix[sizeof "-YYYYMMDD"];
  std::strftime( recsuffix, sizeof recsuffix, "-%Y%m%d", gmtime( &firsttime ) );
  filename = filestart + recsuffix + ".zl";

  std::ofstream out( filename );
  out << "HEADER" << std::endl;
  for ( auto& m : info.metadata( ) ) {
    out << m.first << "=" << m.second << std::endl;
  }

  int sigcount = info.vitals( ).size( ) + info.waves( ).size( );
  std::set<std::string> empties;
  while ( empties.size( ) < sigcount ) {
    time_t nextearliesttime = std::numeric_limits<time_t>::max( );

    out << "TIME " << firsttime << std::endl;
    for ( int i = 0; i < info.vitals( ).size( ); i++ ) {
      std::string& label = vls[i];

      if ( vits[i]->time == firsttime ) {
        out << "VITAL " << label
            << "|" << info.vitals( )[label]->metas( )[SignalData::UOM]
            << "|" << vits[i]->data
            << "|" << vits[i]->high
            << "|" << vits[i]->low
            << std::endl;
        if ( 0 == info.vitals( )[label]->size( ) ) {
          empties.insert( label );
        }
        else {
          vits[i] = std::move( info.vitals( )[label]->pop( ) );
          if ( vits[i]->time < nextearliesttime ) {
            nextearliesttime = vits[i]->time;
          }
        }
      }
    }
    for ( int i = 0; i < info.waves( ).size( ); i++ ) {
      std::string& label = ws[i];

      if ( wavs[i]->time == firsttime ) {
        out << "WAVE " << label
            << "|" << info.waves( )[label]->metas( )[SignalData::UOM]
            << "|" << wavs[i]->data
            << std::endl;
        if ( 0 == info.waves( )[label]->size( ) ) {
          empties.insert( label );
        }
        else {
          wavs[i] = std::move( info.waves( )[label]->pop( ) );
          if ( wavs[i]->time < nextearliesttime ) {
            nextearliesttime = wavs[i]->time;
          }
        }
      }
    }

    firsttime = nextearliesttime;
  }

  out.close( );

  return 0;
}
