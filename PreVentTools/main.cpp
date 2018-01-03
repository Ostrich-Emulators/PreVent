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

void helpAndExit( char * progname, std::string msg = "" ) {
  std::cerr << msg << std::endl
        << "Syntax: " << progname << " <input hdf5>"
        << std::endl << "\t-m or --mrn <mrn>"
        << std::endl << "\t-n or --name <patient name>"
        << std::endl << "\t-o or --output <output file>"
        << std::endl << "\t-a or --attr <key=value>"
        << std::endl << "\t-C or --clobber\toverwrite input file"
        << std::endl;
  exit( 1 );
}

struct option longopts[] = {
  { "mrn", required_argument, NULL, 'm' },
  { "name", required_argument, NULL, 'n' },
  { "clobber", no_argument, NULL, 'C' },
  { "output", required_argument, NULL, 'o' },
  { "attr", required_argument, NULL, 'a' },
  { 0, 0, 0, 0 }
};

void cloneFile( std::unique_ptr<H5::H5File>&infile,
      std::unique_ptr<H5::H5File>& outfile ) {
  hid_t ocpypl_id = H5Pcreate( H5P_OBJECT_COPY );
  // FIXME: iterate through whatever's in the file
  H5Ocopy( infile->getId( ), "/Events", outfile->getId( ), "/Events", ocpypl_id, H5P_DEFAULT );
  H5Ocopy( infile->getId( ), "/Vital Signs", outfile->getId( ), "/Vital Signs", ocpypl_id, H5P_DEFAULT );
  H5Ocopy( infile->getId( ), "/Waveforms", outfile->getId( ), "/Waveforms", ocpypl_id, H5P_DEFAULT );
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

  while ( ( c = getopt_long( argc, argv, ":m:n:o:Ca:", longopts, NULL ) ) != -1 ) {
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

  std::unique_ptr<H5::H5File> infile;
  std::unique_ptr<H5::H5File> outfile;
  std::string infilename = argv[optind];

  if ( "" == outfilename ) {
    // infile and outfile are the same
    if ( clobber ) {
      outfile.reset( new H5::H5File( outfilename, H5F_ACC_RDWR ) );
    }
    else {
      std::cerr << "will not overwrite " << infilename << " (use --clobber)" << std::endl;
      exit( 1 );
    }
  }
  else {
    // if file exists,  worry about clobbering it
    struct stat buffer;
    if ( stat( outfilename.c_str( ), &buffer ) == 0 ) {
      // file exists
      if ( !clobber ) {
        std::cerr << "will not overwrite " << outfilename << " (use --clobber)" << std::endl;
        exit( 1 );
      }
    }

    infile.reset( new H5::H5File( infilename, H5F_ACC_RDONLY ) );
    outfile.reset( new H5::H5File( outfilename, H5F_ACC_TRUNC ) );
    cloneFile( infile, outfile );
  }

  if ( attrs.empty( ) ) {
    std::cout << "yup...that's a file" << std::endl;
  }

  writeAttrs( outfile, attrs );

  outfile->close( );

  return 0;
}
