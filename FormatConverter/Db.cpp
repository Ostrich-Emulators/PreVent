/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Db.h"
#include "SignalSet.h"

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
  std::cout<<"file completed: "<<filename<<std::endl;
}

void Db::onConversionCompleted( const std::string& input, 
    const std::vector<std::string>& outputs ) {
  std::cout<<"conversion completed: "<<input<<std::endl;
}
