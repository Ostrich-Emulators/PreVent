/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Db.h"
#include "SignalSet.h"
#include "SignalData.h"

#include <sys/stat.h>
#include <iostream>

const std::string Db::CREATE = "CREATE TABLE patient (  id INTEGER PRIMARY KEY,  patientname VARCHAR( 500 ));CREATE TABLE unit (  id INTEGER PRIMARY KEY,  name VARCHAR( 25 ));CREATE TABLE bed ( id INTEGER PRIMARY KEY,  unit_id INTEGER,  name VARCHAR( 25 ));CREATE TABLE file (  id INTEGER PRIMARY KEY,  filename VARCHAR( 500 ),  patient_id INTEGER,  bed_id INTEGER,  start INTEGER,  end INTEGER);CREATE TABLE signal (  id INTEGER PRIMARY KEY,  name VARCHAR( 25 ),  hz FLOAT,  uom VARCHAR( 25 ));CREATE TABLE file_signal (  file_id INTEGER,  signal_id INTEGER,  start INTEGER,  end INTEGER,  PRIMARY KEY( file_id, signal_id ));";

int Db::nameidcb( void *a_param, int argc, char **argv, char **column ) {
  std::map<std::string, int> map = ( std::map<std::string, int> ) *a_param;
  map.insert( std::make_pair( argv[0], std::stoi( argv[1] ) ) );
  return 0;
}

int Db::bedcb( void *a_param, int argc, char **argv, char **column ) {
  std::map<std::pair<std::string, std::string>, int> map
      = ( std::map<std::pair<std::string, std::string>, int> ) *a_param;
  map.insert( std::make_pair( std::make_pair( argv[0], argv[1] ), std::stoi( argv[2] ) ) );
  return 0;
}

Db::Db( ) : ptr( nullptr ) {
}

Db::~Db( ) {
  if ( nullptr != ptr ) {

    sqlite3_close( ptr );
  }
}

void Db::init( const std::string& fileloc ) {
  struct stat buffer;

  bool docreate = ( 0 != stat( fileloc.c_str( ), &buffer ) );
  int rc = sqlite3_open( fileloc.c_str( ), &ptr );
  if ( rc ) {
    sqlite3_close( ptr );
    throw sqlite3_errmsg( ptr );
  }

  if ( docreate ) {
    exec( Db::CREATE.c_str( ) );
  }

  exec( "SELECT name, id FROM unit", nameidcb, unitids );

  exec( "SELECT u.name, b.name, b.id FROM bed b JOIN unit u ON b.unit_id=u.id",
      bedcb, bedids );

  exec( "SELECT name, id FROM signal", nameidcb, signalids );

  exec( "SELECT name, id FROM patient", nameidcb, patientids );
}

void Db::exec( const std::string& sql, void * cb, void * param ) {
  char * zErrMsg = nullptr;
  int rc = sqlite3_exec( ptr, sql.c_str( ), cb, param, &zErrMsg );
  if ( rc != SQLITE_OK ) {
    std::string err( zErrMsg );
    sqlite3_free( zErrMsg );
    throw err;
  }
}

int Db::addPatient( const std::string& name ) {
  std::string sql( "INSERT INTO patient( name ) VALUES( ? )" );
  sqlite3_stmt * stmt = nullptr;
  int rc = sqlite3_prepare_v2( ptr, sql.c_str( ), sql.length( ), &stmt, nullptr );
  if ( rc != SQLITE_OK ) {
    sqlite3_finalize( stmt );
    throw "error";
  }

  rc = sqlite3_bind_text( stmt, 1, name.c_str( ), name.length( ), nullptr );
  if ( rc != SQLITE_OK ) {
    sqlite3_finalize( stmt );
    throw "error";
  }

  rc = sqlite3_step( stmt )
  if ( rc != SQLITE_DONE ) {
    sqlite3_finalize( stmt );
    throw "Could not step (execute) stmt";
  }

  int id = sqlite3_last_insert_rowid( ptr );
  patientids.insert( std::make_pair( name, id ) );
  return id;
}

void Db::addFile( const SignalSet& sig ) {
  int pid = 0;
  if ( 0 != sig.metadata( ).count( "Patient Name" ) ) {
    std::string pname = sig.metadata( ).at( "Patient Name" );
    pid = ( 0 == patientids.count( pname )
        ? addPatient( pname )
        : patientids.at( pname ) );
  }

  int rc = sqlite3_exec( ptr, "INSERT INTO file( bedSELECT name, id FROM patient", nameidcb, patientids, &zErrMsg );
  if ( rc != SQLITE_OK ) {
    std::string err( zErrMsg );
    sqlite3_free( zErrMsg );
    throw err;
  }

}

void Db::onFileCompleted( const std::string& filename, const SignalSet& data ) {

  addFile( filename );


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
}
