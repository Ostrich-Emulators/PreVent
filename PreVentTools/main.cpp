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
#include <fstream>

#include <H5Cpp.h>
#include <H5Opublic.h>
#include <vector>
#include <algorithm>
#include <cmath>

#include "H5Cat.h"
#include "dr_time.h"
#include "TimeParser.h"
#include "Writer.h"
#include "Reader.h"
#include "FileNamer.h"
#include "BasicSignalSet.h"
#include "AnonymizingSignalSet.h"
#include "AttributeUtils.h"
#include "OutputSignalData.h"
#include "StatisticalSignalData.h"
#include "NullSignalData.h"
#include "Hdf5Reader.h"

using namespace FormatConverter;

void helpAndExit( char * progname, std::string msg = "" ) {
  std::cerr << msg << std::endl
      << "Syntax: " << progname << " [options] <input hdf5>"
      << std::endl << "\toptions:"
      << std::endl << "\t-o or --output <output file>"
      << std::endl << "\t-S or --set-attr <key[:<i|s|d>]=value>\tsets the given attribute to the value"
      << std::endl << "\t-C or --clobber\toverwrite input file"
      << std::endl << "\t-c or --cat\tconcatenate files from command line, used with --output"
      << std::endl << "\t-s or --start <time>\tstart output from this UTC time (many time formats supported)"
      << std::endl << "\t-e or --end <time>\tstop output immediately before this UTC time (many time formats supported)"
      << std::endl << "\t-f or --for <s>\toutput this many seconds of data from the start of file (or --start)"
      << std::endl << "\t-a or --anonymize, --anon, or --anonymous"
      << std::endl << "\t-p or --path\tsets the path for --set-attr and --attrs"
      << std::endl << "\t-d or --print\tprints data from the path given with --path"
      << std::endl << "\t-A or --attrs\tprints all attributes in the file"
      << std::endl << "\t-V or --vitals\tprints a list of vital signs in this file"
      << std::endl << "\t-W or --waves\tprints a list of waveforms in this file"
      << std::endl << "\t-D or --statistics or --stats\tcalculates descriptive statistics"
      << std::endl;
  exit( 1 );
}

struct option longopts[] = {
  { "clobber", no_argument, NULL, 'C' },
  { "output", required_argument, NULL, 'o' },
  { "set-attr", required_argument, NULL, 'S' },
  { "path", required_argument, NULL, 'p' },
  { "attrs", optional_argument, NULL, 'A' },
  { "start", required_argument, NULL, 's' },
  { "end", required_argument, NULL, 'e' },
  { "for", required_argument, NULL, 'f' },
  { "anonymize", no_argument, NULL, 'a' },
  { "anonymous", no_argument, NULL, 'a' },
  { "print", no_argument, NULL, 'd' },
  { "waves", no_argument, NULL, 'W' },
  { "vitals", no_argument, NULL, 'V' },
  { "cat", no_argument, NULL, 'c' },
  { "stats", no_argument, NULL, 'D' },
  { "statistics", no_argument, NULL, 'D' },
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

  for ( int i = 0; i < infile->getNumAttrs( ); i++ ) {
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
  bool dotime = false;
  bool havestarttime = false;
  dr_time starttime = 0;
  dr_time endtime = std::numeric_limits<dr_time>::max( );
  int for_s = -1;
  bool anon = false;
  bool printattrs = false;
  std::string path = "/";
  bool print = false;
  bool listwaves = false;
  bool listvitals = false;
  bool calc = false;
  bool needsoutput = false;

  while ( ( c = getopt_long( argc, argv, ":o:CAc:s:e:f:aS:dp:WVD", longopts, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'o':
        outfilename = optarg;
        break;
      case 'p':
        path = optarg;
        break;
      case'd':
        print = true;
        break;
      case 'A':
        printattrs = true;
        break;
      case 'W':
        listwaves = true;
        break;
      case 'V':
        listvitals = true;
        break;
      case 'D':
        calc = true;
        break;
      case 'S':
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
      case 'a':
        anon = true;
        needsoutput = true;
        break;
      case 'C':
        clobber = true;
        break;
      case 'c':
        catfiles = true;
        needsoutput = true;
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


  if ( needsoutput ) {
    if ( outfilename.empty( ) ) {
      helpAndExit( argv[0], "please specify an output filename with --output" );
    }

    struct stat buffer;
    if ( stat( outfilename.c_str( ), &buffer ) == 0 && !clobber ) {
      std::stringstream ss;
      ss << "will not overwrite " << outfilename << " (use --clobber)";
      helpAndExit( argv[0], ss.str( ) );
    }
  }

  // just in case an exception gets thrown...
  H5::Exception::dontPrint( );
  if ( printattrs ) {
    for ( int i = optind; i < argc; i++ ) {
      try {
        H5::H5File file = H5::H5File( argv[i], H5F_ACC_RDONLY );
        AttributeUtils::printAttributes( file, path );
      }
      catch ( H5::FileIException& error ) {
        std::cerr << error.getDetailMsg( ) << std::endl;
        return -1;
      }
      // catch failure caused by the DataSet operations
      catch ( H5::DataSetIException& error ) {
        std::cerr << error.getDetailMsg( ) << std::endl;
        return -2;
      }
    }
  }
  else if ( catfiles ) {
    std::vector<std::string> filesToCat;

    for ( int i = optind; i < argc; i++ ) {
      filesToCat.push_back( argv[i] );
    }

    std::cout << "catting " << filesToCat.size( ) << " files to " << outfilename << std::endl;
    //for ( auto x : filesToCat ) {
    //  std::cout << "file to cat: " << x << std::endl;
    //}


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
  else if ( anon ) {
    FileNamer namer = FileNamer::parse( outfilename );

    for ( int i = optind; i < argc; i++ ) {
      std::string input = argv[i];
      std::unique_ptr<Reader> from = Reader::get( FormatConverter::Formats::guess( input ) );
      from->setNonbreaking( true );

      std::unique_ptr<Writer> to = Writer::get( FormatConverter::Formats::guess( input ) );
      namer.tofmt( to->ext( ) );
      namer.inputfilename( input );
      to->filenamer( namer );

      std::unique_ptr<SignalSet>data( new AnonymizingSignalSet( to->filenamer( ) ) );

      if ( from->prepare( input, data ) < 0 ) {
        std::cerr << "could not prepare file for reading: " << input << std::endl;
        continue;
      }
      else {
        std::vector<std::string> files = to->write( from, data );
        from->finish( );

        for ( const auto& f : files ) {
          std::cout << " written to " << f << std::endl;
        }
      }
    }
  }
  else if ( !attrs.empty( ) ) {
    // set attribute values
    for ( int i = optind; i < argc; i++ ) {
      std::string val;
      std::string attrtype( "s" );
      try {
        H5::H5File file = H5::H5File( argv[i], H5F_ACC_RDWR );
        for ( auto& x : attrs ) {
          std::string key = x.first;
          val = x.second;

          size_t delim = key.find( ":" );

          std::string attr( key );
          if ( std::string::npos != delim ) {
            attr = key.substr( 0, delim );
            attrtype = key.substr( delim + 1 );
          }

          if ( "s" == attrtype || "" == val ) {
            AttributeUtils::setAttribute( file, path, attr, val );
          }
          else if ( "i" == attrtype ) {
            AttributeUtils::setAttribute( file, path, attr, std::stoi( val ) );
          }
          else if ( "d" == attrtype ) {
            AttributeUtils::setAttribute( file, path, attr, std::stod( val ) );
          }
        }
      }
      catch ( H5::FileIException& error ) {
        std::cerr << error.getDetailMsg( ) << std::endl;
        return -1;
      }
      // catch failure caused by the DataSet operations
      catch ( H5::DataSetIException& error ) {
        std::cerr << error.getDetailMsg( ) << std::endl;
        return -2;
      }
      catch ( std::invalid_argument& error ) {
        std::cerr << "could not convert \"" << val << "\" to appropriate datatype (" << attrtype << ")" << std::endl;
        return -3;
      }
    }
  }
  else if ( print ) {
    for ( int i = optind; i < argc; i++ ) {
      std::string input = argv[i];

      std::ostream& outstream = ( outfilename.empty( )
          ? std::cout
          : *( new std::ofstream( outfilename ) ) );
      std::unique_ptr<SignalData> signal( new OutputSignalData( outstream ) );

      auto fmt = FormatConverter::Formats::guess( input );
      std::unique_ptr<Reader> rdr = Reader::get( fmt );
      rdr->splice( input, path, starttime, endtime, signal );

      if ( !outfilename.empty( ) ) {
        delete &outstream;
      }
    }
  }
  else if ( listvitals || listwaves ) {

    std::vector<std::string> tolist;
    if ( listvitals ) {
      tolist.push_back( "/VitalSigns" );
    }
    if ( listwaves ) {
      tolist.push_back( "/Waveforms" );
    }

    for ( int j = optind; j < argc; j++ ) {
      H5::H5File file = H5::H5File( argv[j], H5F_ACC_RDONLY );
      for ( auto groupname : tolist ) {
        H5::Group grp = file.openGroup( groupname );
        for ( hsize_t i = 0; i < grp.getNumObjs( ); i++ ) {
          std::string name = grp.getObjnameByIdx( i );
          std::cout << groupname << "\t" << name << std::endl;
        }
      }
    }
  }
  else if ( calc ) {
    std::string input = argv[optind];
    if ( "/" == path ) {
      helpAndExit( argv[0], "--statistics must be accompanied by --path" );
    }

    bool iswave = ( std::string::npos == path.find( "VitalSigns" ) );
    auto fmt = FormatConverter::Formats::guess( input );
    std::unique_ptr<Reader> rdr = Reader::get( fmt );

    StatisticalSignalData * descriptives = new StatisticalSignalData( new NullSignalData( "-", iswave ) );
    std::unique_ptr<SignalData> signal( descriptives );

    if ( for_s > 0 ) {
      if ( !havestarttime ) {
        // need to figure out start time of signal
        std::map<std::string, int> mapi;
        std::map<std::string, std::string> maps;
        std::map<std::string, double> mapd;
        dr_time eendd;
        rdr->getAttributes( input, path, mapi, mapd, maps, starttime, eendd );
      }
      endtime = starttime + for_s * 1000;
    }

    rdr->splice( input, path, starttime, endtime, signal );

    std::cout
        << "count: " << descriptives->count( ) << std::endl
        << "min: " << descriptives->min( ) << std::endl
        << "max: " << descriptives->max( ) << std::endl
        << "median: " << descriptives->median( ) << std::endl
        << "mode: " << descriptives->mode( ) << std::endl
        << "mean: " << descriptives->mean( ) << std::endl
        << "variance: " << descriptives->variance( ) << std::endl
        << "std dev: " << descriptives->stddev( ) << std::endl
        ;
  }
  else {
    // something to acknowledge the program did something
    // (even if the user didn't ask us to do anything)
    std::cout << "yup...that's a file" << std::endl;
  }

  return 0;
}
