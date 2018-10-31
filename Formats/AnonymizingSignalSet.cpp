/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "AnonymizingSignalSet.h"
#include "BasicSignalSet.h"

#include <iostream>
#include <fstream>
#include "config.h"

const std::string AnonymizingSignalSet::DEFAULT_FILENAME_PATTERN = "%i-anonymized-storage.txt";

class AnonymizingSignalData : public SignalDataWrapper{
public:
  AnonymizingSignalData( SignalData * data, dr_time& firsttime );
  AnonymizingSignalData( const std::unique_ptr<SignalData>& data, dr_time& firsttime );
  virtual ~AnonymizingSignalData( );

  virtual void add( const DataRow& row ) override;

private:
  dr_time& firsttime;
};

AnonymizingSignalSet::AnonymizingSignalSet( FileNamer& filenamer )
: SignalSetWrapper( new BasicSignalSet( ) ), namer( filenamer ), firsttime( std::numeric_limits<long>::max( ) ) {
}

AnonymizingSignalSet::AnonymizingSignalSet( const std::unique_ptr<SignalSet>& w,
    FileNamer& filenamer ) : SignalSetWrapper( w ), namer( filenamer ), firsttime( std::numeric_limits<long>::max( ) ) {
}

AnonymizingSignalSet::AnonymizingSignalSet( SignalSet * w, FileNamer& filenamer )
: SignalSetWrapper( w ), namer( filenamer ), firsttime( std::numeric_limits<long>::max( ) ) {
}

AnonymizingSignalSet::~AnonymizingSignalSet( ) {
}

std::unique_ptr<SignalData>& AnonymizingSignalSet::addVital( const std::string& name, bool * added ) {
  bool realadd;
  std::unique_ptr<SignalData>& data = SignalSetWrapper::addVital( name, &realadd );
  if ( realadd ) {
    data.reset( new AnonymizingSignalData( data.release( ), firsttime ) );
  }

  if ( nullptr != added ) {
    *added = realadd;
  }

  return data;
}

std::unique_ptr<SignalData>& AnonymizingSignalSet::addWave( const std::string& name, bool * added ) {
  bool realadd;
  std::unique_ptr<SignalData>& data = SignalSetWrapper::addWave( name, &realadd );
  if ( realadd ) {
    data.reset( new AnonymizingSignalData( data.release( ), firsttime ) );
  }

  if ( nullptr != added ) {
    *added = realadd;
  }

  return data;
}

void AnonymizingSignalSet::setMeta( const std::string& key, const std::string& val ) {
  if ( "Patient Name" == key || "MRN" == key || "Unit" == key || "Bed" == key ) {
    saveddata[key] = val;
  }
  else {
    SignalSetWrapper::setMeta( key, val );
  }
}

void AnonymizingSignalSet::complete( ) {
  // We want to create a separate file with whatever data was anonymized away
  // We'll use a FileNamer to figure out where our output directory is, 
  // and then use another one to generate a filename based on a new pattern.
  // then, we'll just write the saved data to that file

  std::unique_ptr<SignalSet> uniquer( this );
  std::string storeagefile = namer.filename( uniquer );

  size_t pos = storeagefile.find_last_of( "/\\" );

  // make a new filenaming pattern in the output directory
  FileNamer myNamer = FileNamer::parse( ( std::string::npos == pos
      ? ""
      : storeagefile.substr( 0, pos + 1 ) ) + DEFAULT_FILENAME_PATTERN );
  myNamer.inputfilename( namer.inputfilename( ) );
  storeagefile = myNamer.filename( uniquer );
  uniquer.release( ); // don't delete this!

  std::ofstream myfile( storeagefile );
  if ( myfile.is_open( ) ) {
    std::cout << "writing anonymous storage to " << storeagefile << std::endl;
    myfile << namer.inputfilename( ) << std::endl;
    for ( auto& x : saveddata ) {
      myfile << x.first << " = " << x.second << std::endl;
    }
    myfile << "initial time = " << firsttime << std::endl;

    myfile << std::endl;
    myfile.close( );
  }
  else {
    std::cerr << "could not open anonymous storage file: " << storeagefile << std::endl;
  }
}

AnonymizingSignalData::AnonymizingSignalData( SignalData * data, dr_time& first )
: SignalDataWrapper( data ), firsttime( first ) {
}

AnonymizingSignalData::AnonymizingSignalData( const std::unique_ptr<SignalData>& data, dr_time& first )
: SignalDataWrapper( data ), firsttime( first ) {
}

AnonymizingSignalData::~AnonymizingSignalData( ) {
}

void AnonymizingSignalData::add( const DataRow& row ) {
  if ( std::numeric_limits<long>::max( ) == firsttime ) {
    firsttime = row.time;
  }

  DataRow newrow( row );
  newrow.time -= firsttime;
  SignalDataWrapper::add( newrow );
}