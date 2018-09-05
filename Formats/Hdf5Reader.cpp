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

const std::set<std::string> Hdf5Reader::IGNORABLE_PROPS({ "Duration", "End Date/Time",
  "Start Date/Time", "End Time", "Start Time", SignalData::SCALE, SignalData::MSM,
  "Layout Version", "HDF5 Version",
  "Columns", SignalData::TIMEZONE, SignalData::LABEL, "Source Reader" } );

Hdf5Reader::Hdf5Reader( ) : Reader( "HDF5" ) {

}

Hdf5Reader::Hdf5Reader( const Hdf5Reader& ) : Reader( "HDF5" ) {

}

Hdf5Reader::~Hdf5Reader( ) {
}

int Hdf5Reader::prepare( const std::string& filename, std::unique_ptr<SignalSet>& info ) {
  H5::Exception::dontPrint( );
  int rslt = Reader::prepare( filename, info );
  if ( 0 == rslt ) {
    try {
      file = H5::H5File( filename, H5F_ACC_RDONLY );
    }
    catch ( H5::FileIException error ) {
      output( ) << error.getDetailMsg( ) << std::endl;
      return -1;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException error ) {
      output( ) << error.getDetailMsg( ) << std::endl;
      return -2;
    }
  }
  return rslt;
}

void Hdf5Reader::finish( ) {
  file.close( );
}

ReadResult Hdf5Reader::fill( std::unique_ptr<SignalSet>& info, const ReadResult& ) {
  H5::Group root = file.openGroup( "/" );

  for ( int i = 0; i < root.getNumAttrs( ); i++ ) {
    H5::Attribute a = root.openAttribute( i );

    // std::cout << a.getName( ) << ": " << aval << std::endl;
    if ( 0 == IGNORABLE_PROPS.count( a.getName( ) ) ) {
      info->setMeta( a.getName( ), metastr( a ) );
    }
  }

  try {
    H5::Group vgroup = file.openGroup( "/VitalSigns" );
    for ( size_t i = 0; i < vgroup.getNumObjs( ); i++ ) {
      std::string vital = vgroup.getObjnameByIdx( i );
      H5::Group dataAndTimeGroup = vgroup.openGroup( vital );
      readDataSet( dataAndTimeGroup, false, info );
    }
    vgroup.close( );
  }
  catch ( H5::FileIException error ) {
    output( ) << "/VitalSigns: " << error.getDetailMsg( ) << std::endl;
  }
  // catch failure caused by the DataSet operations
  catch ( H5::DataSetIException error ) {
    output( ) << "/VitalSigns: " << error.getDetailMsg( ) << std::endl;
  }

  try {
    H5::Group wgroup = file.openGroup( "/Waveforms" );
    for ( size_t i = 0; i < wgroup.getNumObjs( ); i++ ) {
      std::string wave = wgroup.getObjnameByIdx( i );
      H5::Group dataAndTimeGroup = wgroup.openGroup( wave );
      readDataSet( dataAndTimeGroup, true, info );
    }
    wgroup.close( );
  }
  catch ( H5::FileIException error ) {
    output( ) << "/Waveforms: " << error.getDetailMsg( ) << std::endl;
  }
  // catch failure caused by the DataSet operations
  catch ( H5::DataSetIException error ) {
    output( ) << "/Waveforms: " << error.getDetailMsg( ) << std::endl;
  }

  return ReadResult::END_OF_FILE;
}

size_t Hdf5Reader::getSize( const std::string& input ) const {
  return 1;
}

std::vector<dr_time> Hdf5Reader::readTimes( H5::DataSet& dataset ) const {
  //std::cout << group.getObjName( ) << " " << name << std::endl;
  H5::DataSpace dataspace = dataset.getSpace( );
  hsize_t DIMS[2] = { };
  dataspace.getSimpleExtentDims( DIMS );
  const hsize_t ROWS = DIMS[0];
  const hsize_t COLS = DIMS[1];
  //std::cout << "dimensions: " << DIMS[0] << " " << DIMS[1] << std::endl;

  const hsize_t sizer = ROWS * COLS;
  long read[sizer] = { };
  dataset.read( read, dataset.getDataType( ) );
  std::vector<dr_time> times;
  times.reserve( sizer );
  for ( hsize_t i = 0; i < sizer; i++ ) {
    long l = read[i];
    times.push_back( l );
  }
  //std::cout << "times vector size is: " << times.size( ) << std::endl;
  //std::cout << "first/last vals: " << times[0] << " " << times[times.size( ) - 1] << std::endl;
  return times;
}

void Hdf5Reader::readDataSet( H5::Group& dataAndTimeGroup,
    const bool& iswave, std::unique_ptr<SignalSet>& info ) const {
  std::string name = metastr( dataAndTimeGroup, SignalData::LABEL );

  std::unique_ptr<SignalData>& signal = ( iswave
      ? info->addWave( name )
      : info->addVital( name ) );
  int timeinterval = 2000;
  if ( dataAndTimeGroup.attrExists( SignalData::CHUNK_INTERVAL_MS ) ) {
    timeinterval = metaint( dataAndTimeGroup, SignalData::CHUNK_INTERVAL_MS );
    signal->setMeta(SignalData::CHUNK_INTERVAL_MS, timeinterval);
  }
  int valsPerChunk = 1;
  if ( dataAndTimeGroup.attrExists( SignalData::READINGS_PER_CHUNK ) ) {
    valsPerChunk = metaint( dataAndTimeGroup, SignalData::READINGS_PER_CHUNK );
    signal->setMeta( SignalData::READINGS_PER_CHUNK, valsPerChunk );
  }

  H5::DataSet dataset = dataAndTimeGroup.openDataSet( "data" );

  copymetas( signal, dataset );
  int scale = signal->scale( );
  //for ( auto& x : IGNORABLE_PROPS ) {
    // FIXME: this is a problem...we need to remove props so we don't get an error
    //signal->metad( ).erase( x );
    //signal->metas( ).erase( x );
    //signal->metai( ).erase( x );
  //}

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

  H5::DataSet ds = dataAndTimeGroup.openDataSet( "time" );
  std::vector<dr_time> times = readTimes( ds );
  ds.close( );

  if ( iswave ) {
    fillWave( signal, dataset, times, timeinterval, scale );
  }
  else {
    fillVital( signal, dataset, times, timeinterval, scale );
  }
  dataset.close( );
}

void Hdf5Reader::fillVital( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
    const std::vector<dr_time>& times, int valsPerTime, int scale ) const {
  H5::DataSpace dataspace = dataset.getSpace( );
  hsize_t DIMS[2] = { };
  dataspace.getSimpleExtentDims( DIMS );
  const hsize_t ROWS = DIMS[0];
  const hsize_t COLS = DIMS[1];

  // FIXME: get "Columns" value so we can parse out any extra fields
  //std::cout << dataset.getObjName()<<" dimensions: " << DIMS[0] << " " << DIMS[1] << std::endl;
  std::map<int, std::string> attrmap;
  if ( COLS > 1 && dataset.attrExists( "Columns" ) ) {
    H5::Attribute attr = dataset.openAttribute( "Columns" );
    std::string attrval;
    attr.read( attr.getDataType( ), attrval );
    std::stringstream stream( attrval );
    int col = 0;
    for ( std::string each; std::getline( stream, each, ',' ); ) {
      attrmap[col++] = each;
    }
  }


  // just read everything all at once...in the future, we probably want to
  // worry about using hyperslabs
  short read[ROWS][COLS] = { };
  dataset.read( read, dataset.getDataType( ) );
  for ( size_t row = 0; row < ROWS; row++ ) {
    // FIXME: we better hope valsPerTime is always 1!
    DataRow drow( times[row / valsPerTime], std::to_string( read[row][0] / scale ) );
    if ( COLS > 1 ) {
      for ( size_t c = 1; c < COLS; c++ ) {
        drow.extras[attrmap[c]] = std::to_string( read[row][c] );
      }
    }
    signal->add( drow );
  }
}

void Hdf5Reader::fillWave( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
    const std::vector<dr_time>& times, int valsPerTime, int scale ) const {
  H5::DataSpace dataspace = dataset.getSpace( );
  hsize_t DIMS[2] = { };
  dataspace.getSimpleExtentDims( DIMS );

  const hsize_t ROWS = DIMS[0];
  const hsize_t COLS = DIMS[1];
  const hsize_t MAXSLABROWS = 4096 * 32;
  hsize_t slabrows = ( MAXSLABROWS > ROWS ? ROWS : MAXSLABROWS );

  hsize_t offset[] = { 0, 0 };
  hsize_t count[] = { slabrows, COLS };
  const hsize_t offset0[] = { 0, 0 };

  std::string values;
  int valcnt = 0;

  short rere[slabrows][COLS] = { };
  int timecounter = 0;
  while ( offset[0] < ROWS ) {
    dataspace.selectHyperslab( H5S_SELECT_SET, count, offset );
    H5::DataSpace memspace( 2, count );
    memspace.selectHyperslab( H5S_SELECT_SET, count, offset0 );
    dataset.read( rere, dataset.getDataType( ), memspace, dataspace );

    for ( size_t row = 0; row < count[0]; row++ ) {
      if ( !values.empty( ) ) {
        values.append( "," );
      }
      values.append( std::to_string( rere[row][0] / scale ) );

      valcnt++;
      if ( valsPerTime == valcnt ) {
        DataRow drow( times[timecounter++], values );
        signal->add( drow );
        values.clear( );
        valcnt = 0;
      }
    }

    // get ready for the next read
    offset[0] += count[0];

    // we can't read past our last row, so only fetch exactly how many we need
    if ( offset[0] + slabrows >= ROWS ) {
      count[0] = ROWS - offset[0];
    }
  }
}

void Hdf5Reader::copymetas( std::unique_ptr<SignalData>& signal,
    H5::DataSet& dataset ) const {
  hsize_t cnt = dataset.getNumAttrs( );

  for ( size_t i = 0; i < cnt; i++ ) {
    H5::Attribute attr = dataset.openAttribute( i );
    H5::DataType type = attr.getDataType( );
    const std::string key = upgradeMetaKey( attr.getName( ) );

    switch ( attr.getTypeClass( ) ) {
      case H5T_INTEGER:
      {
        int inty = 0;
        attr.read( type, &inty );
        signal->setMeta( key, inty );
      }
        break;
      case H5T_FLOAT:
      {
        double dbl = 0;
        attr.read( type, &dbl );
        signal->setMeta( key, dbl );
      }
        break;
      default:
        std::string aval;
        attr.read( type, aval );
        signal->setMeta( key, aval );
    }
  }
}

int Hdf5Reader::metaint( const H5::H5Location& loc, const std::string& attrname ) const {
  int val;
  if ( loc.attrExists( attrname ) ) {
    H5::Attribute attr = loc.openAttribute( attrname );
    attr.read( attr.getDataType( ), &val );
  }

  return val;
}

std::string Hdf5Reader::metastr( const H5::H5Location& loc, const std::string& attrname ) const {
  return metastr( loc.openAttribute( attrname ) );
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
  //updates["Sample Frequency"] = SignalData::HERTZ;
  return ( 0 == updates.count( oldkey ) ? oldkey : updates[oldkey] );
}