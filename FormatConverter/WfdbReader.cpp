/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WfdbReader.h"
#include "DataRow.h"

#include <wfdb/wfdb.h>
#include <iostream>
#include <sys/stat.h>

void WfdbReader::doRead( const std::string& input ) {
  int nsig = isigopen( (char *) ( input.c_str( ) ), NULL, 0 );
  if ( nsig > 0 ) {
    WFDB_Siginfo * siginfo = new WFDB_Siginfo[nsig];

    nsig = isigopen( (char *) ( input.c_str( ) ), siginfo, nsig );
    WFDB_Sample * v = new WFDB_Sample[nsig];

    int code = getvec( v );
    while ( code > 0 ) {
      for ( int j = 0; j < nsig; j++ ) {
        //  const time_t& t, const std::string& d, const std::string& hi,
        // const std::string& lo ) : time( t ), data( d ), high( hi ), low( lo ) {
        DataRow row( 0, std::to_string( v[j] ) );

        std::string uom = ( NULL == siginfo[j].units ? "Uncalib" : siginfo[j].units );
        addVital( siginfo[j].desc, row, uom );
      }

      code = getvec( v );
    }

    if ( -3 == code ) {
      std::cerr << "unexpected end of file" << std::endl;
    }
    else if ( -4 == code ) {
      std::cerr << "invalid checksum" << std::endl;
    }


    delete [] v;
    delete [] siginfo;
  }
}

int WfdbReader::getSize( const std::string& input ) const {
  // input is a record name, so we need to figure out how big that data will
  // eventually be
  int nsig = isigopen( (char *) ( input.c_str( ) ), NULL, 0 );
  if ( nsig < 1 ) {
    return -1;
  }

  WFDB_Siginfo * siginfo = new WFDB_Siginfo[nsig];
  nsig = isigopen( (char *) ( input.c_str( ) ), siginfo, nsig );
  long sz = 0;
  for ( int i = 0; i < nsig; i++ ) {
    sz += siginfo[i].nsamp * siginfo[i].fmt;
  }

  delete [] siginfo;

  return (int) sz;
}