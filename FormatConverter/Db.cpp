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
#include <exception>

const std::string Db::CREATE = "CREATE TABLE patient (  id INTEGER PRIMARY KEY,  name VARCHAR( 500 ));CREATE TABLE unit (  id INTEGER PRIMARY KEY,  name VARCHAR( 25 ));CREATE TABLE bed ( id INTEGER PRIMARY KEY,  unit_id INTEGER,  name VARCHAR( 25 ));CREATE TABLE file (  id INTEGER PRIMARY KEY,  filename VARCHAR( 500 ),  patient_id INTEGER,  bed_id INTEGER,  start INTEGER,  end INTEGER);CREATE TABLE signal (  id INTEGER PRIMARY KEY,  name VARCHAR( 25 ),  hz FLOAT,  uom VARCHAR( 25 ));CREATE TABLE file_signal (  file_id INTEGER,  signal_id INTEGER,  start INTEGER,  end INTEGER,  PRIMARY KEY( file_id, signal_id ));";

int Db::nameidcb( void * a_param, int argc, char **argv, char ** ) {
  std::map<std::string, int> map = *static_cast<std::map< std::string, int>*> ( a_param );
  map.insert( std::make_pair( argv[0], std::stoi( argv[1] ) ) );
  return 0;
}

int Db::bedcb( void *a_param, int argc, char **argv, char ** ) {
  std::map<std::pair<std::string, std::string>, int> map
      = *static_cast<std::map<std::pair<std::string, std::string>, int>*> ( a_param );
  map.insert( std::make_pair( std::make_pair( argv[0], argv[1] ), std::stoi( argv[2] ) ) );
  return 0;
}

int Db::signalcb( void * a_param, int argc, char ** argv, char ** column ) {
  std::map<std::pair<std::string, double>, int> map
      = *static_cast<std::map<std::pair<std::string, double>, int>*> ( a_param );
  map.insert( std::make_pair( std::make_pair( argv[0], std::stod( argv[1] ) ), std::stoi( argv[2] ) ) );
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
    throw std::domain_error( sqlite3_errmsg( ptr ) );
  }

  if ( docreate ) {
    exec( Db::CREATE.c_str( ) );
  }

  // FIXME: these caches aren't working
  exec( "SELECT name, id FROM unit", &nameidcb, &unitids );

  exec( "SELECT u.name, b.name, b.id FROM bed b JOIN unit u ON b.unit_id=u.id",
      &bedcb, &bedids );

  exec( "SELECT name, hz, id FROM signal", &signalcb, &signalids );

  exec( "SELECT name, id FROM patient", &nameidcb, &patientids );
}

void Db::exec( const std::string& sql, int (*cb )(void*, int, char**, char**), void * param ) {
  char * zErrMsg = nullptr;
  int rc = sqlite3_exec( ptr, sql.c_str( ), cb, param, &zErrMsg );
  if ( rc != SQLITE_OK ) {
    std::string err( zErrMsg );
    sqlite3_free( zErrMsg );
    throw std::domain_error( err );
  }
}

int Db::addLookup( const std::string& sql, const std::string& name ) {
  sqlite3_stmt * stmt = nullptr;
  int rc = sqlite3_prepare_v2( ptr, sql.c_str( ), sql.length( ), &stmt, nullptr );
  if ( rc != SQLITE_OK ) {
    sqlite3_finalize( stmt );
    throw std::domain_error( sqlite3_errmsg( ptr ) );
  }

  rc = sqlite3_bind_text( stmt, 1, name.c_str( ), name.length( ), nullptr );
  if ( rc != SQLITE_OK ) {
    sqlite3_finalize( stmt );
    throw std::domain_error( sqlite3_errmsg( ptr ) );
  }

  rc = sqlite3_step( stmt );
  if ( rc != SQLITE_DONE ) {
    sqlite3_finalize( stmt );
    throw std::domain_error( sqlite3_errmsg( ptr ) );
  }

  int id = sqlite3_last_insert_rowid( ptr );
  sqlite3_finalize( stmt );

  return id;
}

int Db::getOrAddPatient( const std::string& name ) {
  if ( 0 == patientids.count( name ) ) {
    int id = addLookup( "INSERT INTO patient( name ) VALUES( ? )", name );
    patientids.insert( std::make_pair( name, id ) );
  }
  return patientids.at( name );
}

int Db::getOrAddUnit( const std::string& name ) {
  if ( 0 == unitids.count( name ) ) {
    int id = addLookup( "INSERT INTO unit( name ) VALUES( ? )", name );
    unitids.insert( std::make_pair( name, id ) );
  }

  return unitids.at( name );
}

int Db::getOrAddSignal( const SignalData& data ) {
  std::string name = data.name( );
  double hz = data.hz( );
  auto pairkey = std::make_pair( name, hz );

  if ( 0 == signalids.count( pairkey ) ) {
    std::string sql = "INSERT INTO signal( name, hz, uom ) VALUES( ?, ?, ? )";
    sqlite3_stmt * stmt = nullptr;
    int rc = sqlite3_prepare_v2( ptr, sql.c_str( ), sql.length( ), &stmt, nullptr );
    if ( rc != SQLITE_OK ) {
      sqlite3_finalize( stmt );
      throw std::domain_error( sqlite3_errmsg( ptr ) );
    }

    sqlite3_bind_text( stmt, 1, name.c_str( ), name.length( ), nullptr );
    sqlite3_bind_double( stmt, 2, hz );

    if ( data.uom( ).empty( ) ) {
      sqlite3_bind_null( stmt, 3 );
    }
    else {
      sqlite3_bind_text( stmt, 3, data.uom( ).c_str( ), data.uom( ).length( ), nullptr );
    }

    rc = sqlite3_step( stmt );
    if ( rc != SQLITE_DONE ) {
      sqlite3_finalize( stmt );
      throw std::domain_error( sqlite3_errmsg( ptr ) );
    }

    int id = sqlite3_last_insert_rowid( ptr );
    sqlite3_finalize( stmt );

    signalids.insert( std::make_pair( pairkey, id ) );
  }

  return signalids.at( pairkey );
}

void Db::addSignal( int fileid, const SignalData& sig ) {
  //CREATE TABLE file_signal (  file_id INTEGER,  signal_id INTEGER,  start INTEGER,  end INTEGER,  PRIMARY KEY( file_id, signal_id ));
  int sid = getOrAddSignal( sig );
  std::string sql = "INSERT INTO file_signal( file_id, signal_id, start, end ) VALUES( ?, ?, ?, ? )";
  sqlite3_stmt * stmt = nullptr;
  int rc = sqlite3_prepare_v2( ptr, sql.c_str( ), sql.length( ), &stmt, nullptr );
  if ( rc != SQLITE_OK ) {
    sqlite3_finalize( stmt );
    throw std::domain_error( sqlite3_errmsg( ptr ) );
  }

  sqlite3_bind_int( stmt, 1, fileid );
  sqlite3_bind_int( stmt, 2, sid );
  sqlite3_bind_int( stmt, 3, sig.startTime( ) );
  sqlite3_bind_int( stmt, 4, sig.endTime( ) );

  rc = sqlite3_step( stmt );
  if ( rc != SQLITE_DONE ) {
    sqlite3_finalize( stmt );
    throw std::domain_error( sqlite3_errmsg( ptr ) );
  }

  sqlite3_finalize( stmt );
}

int Db::getOrAddBed( const std::string& name, const std::string& unitname ) {
  auto pairkey = std::make_pair( unitname, name );

  if ( 0 == bedids.count( pairkey ) ) {
    int unitid = getOrAddUnit( unitname );
    // since we're constructing the SQL from a generated id, it's safe
    // to "inject" one of the bind variables
    int id = addLookup( "INSERT INTO bed( unit_id, name ) VALUES( "
        + std::to_string( unitid ) + ", ? )", name );
    bedids.insert( std::make_pair( pairkey, id ) );
  }

  return bedids.at( pairkey );
}

void Db::setProperty( ConversionProperty key, const std::string& val ) {
  if ( ConversionProperty::QUIET == key ) {
    quiet = ( "TRUE" == val );
  }
}

void Db::onFileCompleted( const std::string& filename, const SignalSet& data ) {
  if ( !quiet ) {
    std::cout << "updating database" << std::endl;
  }

  int pid = 0;
  int bid = 0;
  if ( 0 != data.metadata( ).count( "Patient Name" ) ) {
    std::string pname = data.metadata( ).at( "Patient Name" );
    pid = getOrAddPatient( pname );
  }

  if ( 0 != ( data.metadata( ).count( "Bed" ) + data.metadata( ).count( "Unit" ) ) ) {
    std::string unitname = data.metadata( ).at( "Unit" );
    std::string bedname = data.metadata( ).at( "Bed" );
    bid = getOrAddBed( bedname, unitname );
  }

  std::string sql
      = "INSERT INTO file( filename, bed_id, patient_id, start, end ) VALUES( ?, ?, ?, ?, ? )";
  sqlite3_stmt * stmt = nullptr;
  int rc = sqlite3_prepare_v2( ptr, sql.c_str( ), sql.length( ), &stmt, nullptr );
  if ( rc != SQLITE_OK ) {
    sqlite3_finalize( stmt );
    throw std::domain_error( sqlite3_errmsg( ptr ) );
  }

  sqlite3_bind_text( stmt, 1, filename.c_str( ), filename.length( ), nullptr );
  if ( 0 == bid ) {
    sqlite3_bind_null( stmt, 2 );
  }
  else {
    sqlite3_bind_int( stmt, 2, bid );
  }

  if ( 0 == pid ) {
    sqlite3_bind_null( stmt, 3 );
  }
  else {
    sqlite3_bind_int( stmt, 3, pid );
  }

  sqlite3_bind_int( stmt, 4, data.earliest( ) );
  sqlite3_bind_int( stmt, 5, data.latest( ) );

  rc = sqlite3_step( stmt );
  if ( rc != SQLITE_DONE ) {
    sqlite3_finalize( stmt );
    throw std::domain_error( sqlite3_errmsg( ptr ) );
  }

  int id = sqlite3_last_insert_rowid( ptr );
  sqlite3_finalize( stmt );

  for ( const std::unique_ptr<SignalData>& signal : data.allsignals( ) ) {
    addSignal( id, *signal );
  }

  //  std::cout << "file completed: " << filename << std::endl;
  //
  //  std::cout << "\t" << data.earliest( ) << " to " << data.latest( ) << std::endl;
  //  for ( const auto& m : data.metadata( ) ) {
  //    std::cout << "\t" << m.first << ": " << m.second << std::endl;
  //  }
  //
  //  for ( const std::unique_ptr<SignalData>& m : data.allsignals( ) ) {
  //    std::cout << "\t  " << ( m->wave( ) ? "WAVE " : "VITAL " ) << m->name( ) << std::endl;
  //    std::cout << "\t\t" << m->startTime( ) << " to " << m->endTime( ) << std::endl;
  //
  //    for ( const auto& x : m->metad( ) ) {
  //      std::cout << "\t\t" << x.first << ": " << x.second << std::endl;
  //    }
  //    for ( const auto& x : m->metas( ) ) {
  //      std::cout << "\t\t" << x.first << ": " << x.second << std::endl;
  //    }
  //    for ( const auto& x : m->metai( ) ) {
  //
  //      std::cout << "\t\t" << x.first << ": " << x.second << std::endl;
  //    }
  //  }
}

void Db::onConversionCompleted( const std::string& input,
    const std::vector<std::string>& outputs ) {
}
