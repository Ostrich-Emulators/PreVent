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

namespace FormatConverter {
  const std::string AnonymizingSignalSet::DEFAULT_FILENAME_PATTERN = "%i-anonymized-storage.txt";

  AnonymizingSignalSet::AnonymizingSignalSet( FileNamer& filenamer )
  : SignalSetWrapper( new BasicSignalSet( ) ), namer( filenamer ), timemod( TimeModifier::time( 0 ) ) {
  }

  AnonymizingSignalSet::AnonymizingSignalSet( const std::unique_ptr<SignalSet>& w,
          FileNamer& filenamer, const TimeModifier& tm ) : SignalSetWrapper( w ), namer( filenamer ), timemod( tm ) {
  }

  AnonymizingSignalSet::AnonymizingSignalSet( SignalSet * w, FileNamer& filenamer, const TimeModifier& tm )
  : SignalSetWrapper( w ), namer( filenamer ), timemod( tm ) {
  }

  AnonymizingSignalSet::~AnonymizingSignalSet( ) {
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
    std::string storagefile = namer.filename( uniquer );

    size_t pos = storagefile.find_last_of( "/\\" );

    // make a new filenaming pattern in the output directory
    FileNamer myNamer = FileNamer::parse( ( std::string::npos == pos
            ? ""
            : storagefile.substr( 0, pos + 1 ) ) + DEFAULT_FILENAME_PATTERN );
    myNamer.inputfilename( storagefile );
    storagefile = myNamer.filename( uniquer );
    uniquer.release( ); // don't delete /this/!

    std::ofstream myfile( storagefile );
    if ( myfile.is_open( ) ) {
      std::cout << "writing anonymous storage to " << storagefile << std::endl;
      myfile << namer.inputfilename( ) << std::endl;
      for ( auto& x : saveddata ) {
        myfile << x.first << " = " << x.second << std::endl;
      }
      myfile << "initial time offset = " << timemod.firsttime( ) << std::endl;

      myfile << std::endl;
      myfile.close( );
    }
    else {
      std::cerr << "could not open anonymous storage file: " << storagefile << std::endl;
    }
  }
}
