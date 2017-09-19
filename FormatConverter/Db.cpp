/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Db.h"
#include "SignalSet.h"
#include "SignalData.h"

#include <iostream>

Db::Db( const std::string& fileloc ) {
  ptr = nullptr;
  int rc = sqlite3_open( fileloc.c_str( ), &ptr );
  if ( rc ) {
    sqlite3_close( ptr );
    throw sqlite3_errmsg( ptr );
  }
}

Db::~Db( ) {
  sqlite3_close( ptr );
}

void Db::onFileCompleted( const std::string& filename, const SignalSet& data ) {
  std::cout << "file completed: " << filename << std::endl;

  std::cout << "\t" << data.earliest( ) << " to " << data.latest( ) << std::endl;
  for ( const auto& m : data.metadata( ) ) {
    std::cout << "\t" << m.first << ": " << m.second << std::endl;
  }

  for ( const std::unique_ptr<SignalData>& m : data.allsignals( ) ) {
    std::cout << "\t  " << ( m->wave( ) ? "WAVE " : "VITAL " ) << m->name( ) << std::endl;
    std::cout << "\t\t" << m->startTime( ) << " to " << m->endTime( ) << std::endl;

    for ( const auto& x : m->metad( ) ) {
      std::cout << "\t\t" << x.first << ": " << x.second << std::endl;
    }
    for ( const auto& x : m->metas( ) ) {
      std::cout << "\t\t" << x.first << ": " << x.second << std::endl;
    }
    for ( const auto& x : m->metai( ) ) {
      std::cout << "\t\t" << x.first << ": " << x.second << std::endl;
    }

  }
}

void Db::onConversionCompleted( const std::string& input,
    const std::vector<std::string>& outputs ) {
  std::cout << "conversion completed: " << input << std::endl;
}
