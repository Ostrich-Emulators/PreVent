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
#include <iostream>
#include <getopt.h>

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
  { "clobber", required_argument, NULL, 'C' },
  { "output", required_argument, NULL, 'o' },
  { "attr", required_argument, NULL, 'a' },
  { 0, 0, 0, 0 }
};

/*
 * 
 */
int main( int argc, char** argv ) {
  char c;
  extern int optind;
  extern char * optarg;

  std::string mrn = "";
  bool domrn = false;
  std::string name = "";
  bool doname = false;
  std::string outfile = "";
  bool clobber = false;
  std::map <std::string, std::string> attrs;
  bool doattrs = false;

  while ( ( c = getopt_long( argc, argv, ":m:n:o:Ca:", longopts, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'm':
        mrn = optarg;
        domrn = true;
        break;
      case 'n':
        name = optarg;
        domrn = true;
        break;
      case 'o':
        outfile = optarg;
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
        doattrs = true;
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

  if ( !( domrn || doname || doattrs ) ) {
    std::cout << "yup...that's a file" << std::endl;
  }

  if ( doattrs ) {
    for ( const auto&x : attrs ) {
      std::cout << x.first << " => " << x.second << std::endl;
    }
  }



  return 0;
}

