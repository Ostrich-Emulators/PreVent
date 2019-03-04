/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "AttributeUtils.h"

#include <iostream>
#include <deque>

void AttributeUtils::printAttributes( H5::H5File& file, const std::string& path, bool recursive ) {

  std::deque<std::string> todo;

  todo.push_front( "" == path ? "/" : path );

  bool first = true;
  while ( !todo.empty( ) ) {
    std::string itempath = todo.front( );
    todo.pop_front( );

    H5G_stat_t stats = { };
    file.getObjinfo( itempath, stats );

    if ( stats.type == H5G_GROUP ) {
      H5::Group grp = file.openGroup( itempath );
      iprintAttributes( grp );

      if ( recursive ) {
        for ( hsize_t i = 0; i < grp.getNumObjs( ); i++ ) {
          todo.push_front( itempath + ( first ? "" : "/" ) + grp.getObjnameByIdx( i ) );
        }
      }
    }
    else if ( stats.type == H5G_DATASET ) {
      H5::DataSet ds = file.openDataSet( itempath );
      iprintAttributes( ds );
    }

    first = false;
  }
}

void AttributeUtils::iprintAttributes( H5::H5Object& location ) {
  for ( hsize_t i = 0; i < location.getNumAttrs( ); i++ ) {
    std::cout << location.getObjName( );

    H5::Attribute attr = location.openAttribute( i );
    H5::DataSpace space = H5::DataSpace( H5S_SCALAR );
    H5::DataType dt = attr.getDataType( );

    std::cout << "|" << attr.getName( ) << ":";

    switch ( attr.getDataType( ).getClass( ) ) {
      case H5T_INTEGER:
      {
        int val = 0;
        attr.read( dt, &val );
        std::cout << val;
      }
        break;
      case H5T_FLOAT:
      {
        double val = 0.0;
        attr.read( dt, &val );
        std::cout << val;
      }
        break;
      case H5T_STRING:
      {
        std::string val;
        attr.read( dt, val );
        std::cout << val;
      }
        break;
    }

    attr.close( );
    std::cout << std::endl;
  }
}

