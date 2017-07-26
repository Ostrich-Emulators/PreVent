/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Hdf5Reader.h"

#include "DataRow.h"
#include "SignalData.h"
#include <iostream>
#include <exception>

Hdf5Reader::Hdf5Reader( ) {

}

Hdf5Reader::Hdf5Reader( const Hdf5Reader& ) {

}

Hdf5Reader::~Hdf5Reader( ) {
}

int Hdf5Reader::prepare( const std::string& filename, SignalSet& info ) {
  file = H5::H5File( filename, H5F_ACC_RDONLY );
  return 0;
}

void Hdf5Reader::finish( ) {
  file.close( );
}

ReadResult Hdf5Reader::fill( SignalSet& info, const ReadResult& ) {
  H5::Group root = file.openGroup( "/" );
  for ( int i = 0; i < root.getNumAttrs( ); i++ ) {
    H5::Attribute a = root.openAttribute( i );
    info.addMeta( a.getName( ), metastr( a ) );
    // std::cout << a.getName( ) << ": " << aval << std::endl;
  }

  H5::Group vgroup = file.openGroup( "/Vital Signs" );
  for ( int i = 0; i < vgroup.getNumObjs( ); i++ ) {
    std::string vital = vgroup.getObjnameByIdx( i );
    readDataSet( vgroup, vital, false, info );
  }
  vgroup.close( );


  H5::Group wgroup = file.openGroup( "/Waveforms" );
  for ( int i = 0; i < wgroup.getNumAttrs( ); i++ ) {
    H5::Attribute a = wgroup.openAttribute( i );
    std::cout << "waves a: " << a.getName( ) << std::endl;
  }
  wgroup.close( );

  return ReadResult::END_OF_FILE;
}

int Hdf5Reader::getSize( const std::string& input ) const {
  return 0;
}

void Hdf5Reader::readDataSet( H5::Group& group, const std::string& name,
    const bool& iswave, SignalSet& info ) const {
  std::unique_ptr<SignalData>& signal = info.addVital( name );
  H5::DataSet dataset = group.openDataSet( name );
  copymetas( signal, dataset );
  //  for ( auto& m : signal->metad( ) ) {
  //    std::cout << "  " << m.first << ": (d) " << m.second << std::endl;
  //  }
  //  for ( auto& m : signal->metai( ) ) {
  //    std::cout << "  " << m.first << ": (i) " << m.second << std::endl;
  //  }
  //  auto& strings = signal->metas();
  //  for ( auto& m : signal->metas( ) ) {
  //    std::cout << "  " << m.first << ": (s) " << m.second << std::endl;
  //  }
  //


  if ( iswave ) {

  }
  else {
    fillVital( signal, dataset );
  }
}

void Hdf5Reader::fillVital( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset ) const {
  H5::DataSpace dataspace = dataset.getSpace( );
  const int COLS = 4;
  const hssize_t ROWS = dataspace.getSimpleExtentNpoints( ) / COLS;

  // just read everything all at once...in the future, we probably want to
  // worry about using hyperslabs
  int read[ROWS][COLS] = { 0 };
  dataset.read( read, dataset.getDataType( ) );
  for ( int row = 0; row < ROWS; row++ ) {

    DataRow drow( read[row][0],
        std::to_string( read[row][1] ),
        std::to_string( read[row][2] ),
        std::to_string( read[row][3] ) );
    signal->add( drow );

    //    for ( int j = 0; j < COLS; j++ ) {
    //
    //      std::cout << "  " << read[i][j];
    //    }
    //    std::cout << std::endl;
  }
}

void Hdf5Reader::copymetas( std::unique_ptr<SignalData>& signal,
    H5::DataSet& dataset ) const {
  hsize_t cnt = dataset.getNumAttrs( );

  for ( int i = 0; i < cnt; i++ ) {
    H5::Attribute attr = dataset.openAttribute( i );
    H5::DataType type = attr.getDataType( );
    const std::string key = upgradeMetaKey( attr.getName( ) );

    switch ( attr.getTypeClass( ) ) {
      case H5T_INTEGER:
      {
        int inty = 0;
        attr.read( type, &inty );
        signal->metai( )[key] = inty;
      }
        break;
      case H5T_FLOAT:
      {
        double dbl = 0;
        attr.read( type, &dbl );
        signal->metad( )[key] = dbl;
      }
        break;
      default:
        std::string aval;
        attr.read( type, aval );
        signal->metas( )[key] = aval;
    }
  }
}

std::string Hdf5Reader::metastr( const H5::Attribute& attr ) const {
  H5::DataType type = attr.getDataType( );

  std::string aval;
  switch ( attr.getTypeClass( ) ) {
    case H5T_INTEGER:
    {
      int inty = 0;
      attr.read( type, &inty );
      aval = std::to_string( inty );
    }
      break;
    case H5T_FLOAT:
    {
      double dbl = 0;
      attr.read( type, &dbl );
      aval = std::to_string( dbl );
    }
      break;
    default:
      attr.read( type, aval );
  }

  return aval;
}

std::string Hdf5Reader::upgradeMetaKey( const std::string& oldkey ) const {
  std::map<std::string, std::string> updates;
  updates["Sample Frequency"] = SignalData::HERTZ;

  return ( 0 == updates.count( oldkey ) ? oldkey : updates[oldkey] );
}