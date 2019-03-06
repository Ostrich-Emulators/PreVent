/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "AttributeUtils.h"

#include <iostream>
#include <deque>
#include "Hdf5Writer.h"

void AttributeUtils::printAttributes( H5::H5File& file, const std::string& path, bool recursive ) {

  std::deque<std::string> todo;

  todo.push_front( "" == path ? "/" : path );

  while ( !todo.empty( ) ) {
    std::string itempath = todo.front( );
    todo.pop_front( );

    H5G_stat_t stats = { };
    try {
      file.getObjinfo( itempath, stats );
    }
    catch ( H5::FileIException error ) {
      std::cerr << "could not open dataset/group: " << itempath << std::endl;
      return;
    }

    if ( stats.type == H5G_GROUP ) {
      H5::Group grp = file.openGroup( itempath );
      iprintAttributes( grp );

      if ( recursive ) {
        for ( hsize_t i = 0; i < grp.getNumObjs( ); i++ ) {
          if ( itempath.rfind( "/" ) != itempath.length( ) - 1 ) {
            itempath += "/";
          }

          todo.push_front( itempath + grp.getObjnameByIdx( i ) );
        }
      }
    }
    else if ( stats.type == H5G_DATASET ) {
      H5::DataSet ds = file.openDataSet( itempath );
      iprintAttributes( ds );
    }
  }
}

void AttributeUtils::iprintAttributes( H5::H5Object& location ) {
  for ( hsize_t i = 0; i < location.getNumAttrs( ); i++ ) {
    std::cout << location.getObjName( );

    H5::Attribute attr = location.openAttribute( i );
    H5::DataType dt = attr.getDataType( );

    std::cout << "|" << attr.getName( ) << "|";

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

void AttributeUtils::setAttribute( H5::H5File& file, const std::string& path, const std::string& attr,
    const std::string& val ) {
  H5G_stat_t stats = { };

  try {
    file.getObjinfo( path, stats );
  }
  catch ( H5::FileIException error ) {
    std::cerr << "could not open dataset/group: " << path << std::endl;
    return;
  }

  if ( stats.type == H5G_GROUP ) {
    H5::Group grp = file.openGroup( path );

    if ( grp.attrExists( attr ) ) {
      grp.removeAttr( attr );
    }
    if ( "" == val ) {
      std::cout << attr << " attribute removed from " << path << std::endl;
    }
    else {
      Hdf5Writer::writeAttribute( grp, attr, val );
      std::cout << attr << " attribute written to " << path << std::endl;
    }
  }
  else if ( stats.type == H5G_DATASET ) {
    H5::DataSet ds = file.openDataSet( path );
    if ( ds.attrExists( attr ) ) {
      ds.removeAttr( attr );
    }
    if ( "" == val ) {
      std::cout << attr << " attribute removed from " << path << std::endl;
    }
    else {
      Hdf5Writer::writeAttribute( ds, attr, val );
      std::cout << attr << " attribute written to " << path << std::endl;
    }
  }
}

void AttributeUtils::setAttribute( H5::H5File& file, const std::string& path, const std::string& attr,
    double val ) {
  H5G_stat_t stats = { };

  try {
    file.getObjinfo( path, stats );
  }
  catch ( H5::FileIException error ) {
    std::cerr << "could not open dataset/group: " << path << std::endl;
    return;
  }

  if ( stats.type == H5G_GROUP ) {
    H5::Group grp = file.openGroup( path );

    if ( grp.attrExists( attr ) ) {
      grp.removeAttr( attr );
    }
    Hdf5Writer::writeAttribute( grp, attr, val );
  }
  else if ( stats.type == H5G_DATASET ) {
    H5::DataSet ds = file.openDataSet( path );
    if ( ds.attrExists( attr ) ) {
      ds.removeAttr( attr );
    }
    Hdf5Writer::writeAttribute( ds, attr, val );
  }
  std::cout << attr << " attribute written to " << path << std::endl;
}

void AttributeUtils::setAttribute( H5::H5File& file, const std::string& path, const std::string& attr,
    int val ) {
  H5G_stat_t stats = { };

  try {
    file.getObjinfo( path, stats );
  }
  catch ( H5::FileIException error ) {
    std::cerr << "could not open dataset/group: " << path << std::endl;
    return;
  }

  if ( stats.type == H5G_GROUP ) {
    H5::Group grp = file.openGroup( path );

    if ( grp.attrExists( attr ) ) {
      grp.removeAttr( attr );
    }
    Hdf5Writer::writeAttribute( grp, attr, val );
  }
  else if ( stats.type == H5G_DATASET ) {
    H5::DataSet ds = file.openDataSet( path );
    if ( ds.attrExists( attr ) ) {
      ds.removeAttr( attr );
    }
    Hdf5Writer::writeAttribute( ds, attr, val );
  }
  std::cout << attr << " attribute written to " << path << std::endl;
}
