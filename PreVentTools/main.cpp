/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: ryan
 *
 * Created on January 3, 2018, 8:47 AM
 */

#include <cstdlib>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <getopt.h>
#include <sys/stat.h>

#include <H5Cpp.h>
#include <H5Opublic.h>
#include <vector>
#include <algorithm>

#include "H5Cat.h"
#include "dr_time.h"
#include "TimeParser.h"

void helpAndExit( char * progname, std::string msg = "" ) {
  std::cerr << msg << std::endl
      << "Syntax: " << progname << "[options] <input hdf5>"
      << std::endl << "\toptions:"
      << std::endl << "\t-m or --mrn <mrn>\tsets MRN in file"
      << std::endl << "\t-n or --name <patient name>\tsets name in file"
      << std::endl << "\t-o or --output <output file>"
      << std::endl << "\t-a or --attr <key=value>\tsets the given attribute to the value"
      << std::endl << "\t-C or --clobber\toverwrite input file"
      << std::endl << "\t-c or --cat\tconcatenate files from command line, used with --output"
      << std::endl << "\t-s or --start <time>\tstart output from this UTC time (many time formats supported)"
      << std::endl << "\t-e or --end <time>\tstop output immediately before this UTC time (many time formats supported)"
      << std::endl << "\t-f or --for <s>\toutput this many seconds of data from the start of file (or --start)"
      << std::endl;
  exit( 1 );
}

struct option longopts[] = {
  { "mrn", required_argument, NULL, 'm' },
  { "name", required_argument, NULL, 'n' },
  { "clobber", no_argument, NULL, 'C' },
  { "output", required_argument, NULL, 'o' },
  { "attr", required_argument, NULL, 'a' },
  { "start", required_argument, NULL, 's' },
  { "end", required_argument, NULL, 'e' },
  { "for", required_argument, NULL, 'f' },
  { "cat", no_argument, NULL, 'c' }, // all remaining args are files
  { 0, 0, 0, 0 }
};

void cloneFile( std::unique_ptr<H5::H5File>&infile,
    std::unique_ptr<H5::H5File>& outfile ) {
  hid_t ocpypl_id = H5Pcreate( H5P_OBJECT_COPY );
  for ( hsize_t i = 0; i < infile->getNumObjs( ); i++ ) {
    std::string name = infile->getObjnameByIdx( i );
    H5Ocopy( infile->getId( ), name.c_str( ),
        outfile->getId( ), name.c_str( ),
        ocpypl_id, H5P_DEFAULT );
  }

  for ( hsize_t i = 0; i < infile->getNumAttrs( ); i++ ) {
    H5::Attribute attr = infile->openAttribute( i );
    H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
    H5::DataType dt = attr.getDataType( );

    std::string val;
    attr.read( dt, val );

    H5::Attribute newattr = outfile->createAttribute( attr.getName( ), dt, space );
    newattr.write( dt, val );
    newattr.close( );
    attr.close( );
  }
}

void writeAttrs( std::unique_ptr<H5::H5File>& outfile, std::map<std::string, std::string> attrs ) {
  for ( const auto& kv : attrs ) {
    H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
    H5::StrType st( H5::PredType::C_S1, H5T_VARIABLE );
    st.setCset( H5T_CSET_UTF8 );

    std::string attr = kv.first;
    std::string val = kv.second;

    if ( outfile->attrExists( attr ) ) {
      outfile->removeAttr( attr );
    }

    H5::Attribute attrib = outfile->createAttribute( attr, st, space );
    attrib.write( st, val );
    attrib.close( );
  }
}

/*
 * 
 */
int main( int argc, char** argv ) {
  char c;
  extern int optind;
  extern char * optarg;

  std::string outfilename = "";
  bool clobber = false;
  std::map <std::string, std::string> attrs;
  bool catfiles = false;
  std::vector<std::string> filesToCat;
  bool dotime = false;
  bool havestarttime = false;
  dr_time starttime = 0;
  dr_time endtime = std::numeric_limits<dr_time>::max( );
  int for_s = -1;

  while ( ( c = getopt_long( argc, argv, ":m:n:o:Ca:c:s:e:f:", longopts, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'm':
        attrs["MRN"] = optarg;
        break;
      case 'n':
        attrs["Patient Name"] = optarg;
        break;
      case 'o':
        outfilename = optarg;
        break;
      case 'a':
      {
        std::string kv( optarg );
        size_t idx = kv.find( '=' );
        if ( idx == std::string::npos ) {
          std::cerr << "attributes must be in the form <key>=<value>";
          helpAndExit( argv[0] );
        }
        else {
          attrs[kv.substr( 0, idx )] = kv.substr( idx + 1 );
        }
      }
        break;
      case 'C':
        clobber = true;
        break;
      case 'c':
        catfiles = true;
        break;
      case 's':
        starttime = TimeParser::parse( optarg );
        havestarttime = true;
        dotime = true;
        break;
      case 'e':
        endtime = TimeParser::parse( optarg );
        dotime = true;
        break;
      case 'f':
        for_s = std::stoi( optarg );
        dotime = true;
        break;
      case ':':
        std::cerr << "missing option argument" << std::endl;
        helpAndExit( argv[0] );
        break;
      case '?':
      default:
        helpAndExit( argv[0] );
        break;
    }
  }
  if ( ( optind + 1 ) > argc ) {
    helpAndExit( argv[0], "no file specified" );
  }

  if ( catfiles ) {
    if ( outfilename.empty( ) ) {
      helpAndExit( argv[0], "please specify an output filename with --output" );
    }

    for ( int i = optind; i < argc; i++ ) {
      filesToCat.push_back( argv[i] );
    }

    std::cout << "catting " << filesToCat.size( ) << " files to " << outfilename << std::endl;
    //for ( auto x : filesToCat ) {
    //  std::cout << "file to cat: " << x << std::endl;
    //}

    struct stat buffer;
    if ( stat( outfilename.c_str( ), &buffer ) == 0 && !clobber ) {
      std::cerr << "will not overwrite " << outfilename << " (use --clobber)" << std::endl;
      exit( 1 );
    }

    H5Cat catter( outfilename );
    if ( dotime ) {
      if ( for_s > 0 ) {
        catter.setDuration( for_s * 1000, ( havestarttime ? &starttime : nullptr ) );
      }
      else {
        catter.setClipping( starttime, endtime );
      }
    }
    catter.cat( filesToCat );
  }
  else if ( attrs.empty( ) ) {
    // something to acknowledge the program did something
    // (even if the user didn't ask us to do anything)
    std::cout << "yup...that's a file" << std::endl;
  }
  else { // write some attributes
    std::unique_ptr<H5::H5File> infile;
    std::unique_ptr<H5::H5File> outfile;
    std::string infilename = argv[optind];

    if ( "" == outfilename ) {
      // infile and outfile are the same
      if ( clobber ) {
        outfile.reset( new H5::H5File( infilename, H5F_ACC_RDWR ) );
      }
      else {
        std::cerr << "will not overwrite " << infilename << " (use --clobber)" << std::endl;
        exit( 1 );
      }
    }
    else {
      // if file exists,  worry about clobbering it
      struct stat buffer;
      if ( stat( outfilename.c_str( ), &buffer ) == 0 && !clobber ) {
        std::cerr << "will not overwrite " << outfilename << " (use --clobber)" << std::endl;
        exit( 1 );
      }

      infile.reset( new H5::H5File( infilename, H5F_ACC_RDONLY ) );
      outfile.reset( new H5::H5File( outfilename, H5F_ACC_TRUNC ) );
      cloneFile( infile, outfile );
    }

    writeAttrs( outfile, attrs );
    outfile->close( );
  }

  return 0;
}
