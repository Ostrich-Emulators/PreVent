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
#include <cmath>
#include "BasicSignalData.h"

const std::set<std::string> Hdf5Reader::IGNORABLE_PROPS({ "Duration", "End Date/Time",
  "Start Date/Time", "End Time", "Start Time", SignalData::SCALE, SignalData::MSM,
  "Layout Version", "HDF5 Version", "HDF5 Version", "Layout Version",
  "Columns", SignalData::TIMEZONE, SignalData::LABEL, "Source Reader",
  "Note on Scale" } );

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

bool Hdf5Reader::getAttributes( const std::string& inputfile, std::map<std::string, std::string>& map ) {
  H5::Exception::dontPrint( );
  try {
    file = H5::H5File( inputfile, H5F_ACC_RDONLY );
    H5::Group root = file.openGroup( "/" );

    for ( int i = 0; i < root.getNumAttrs( ); i++ ) {
      H5::Attribute a = root.openAttribute( i );
      map[a.getName( )] = metastr( a );
    }
  }
  catch ( H5::FileIException error ) {
    output( ) << error.getDetailMsg( ) << std::endl;
    file.close( );
    return false;
  }
  // catch failure caused by the DataSet operations
  catch ( H5::DataSetIException error ) {
    output( ) << error.getDetailMsg( ) << std::endl;
    file.close( );
    return false;
  }

  return true;
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
    H5::Group egroup = file.openGroup( "/Events" );
    for ( size_t i = 0; i < egroup.getNumObjs( ); i++ ) {
      std::string ev = egroup.getObjnameByIdx( i );
      if ( "Segment_Offsets" == ev ) {
        H5::DataSet offsets = egroup.openDataSet( ev );
        H5::DataSpace dataspace = offsets.getSpace( );
        hsize_t DIMS[2] = { };
        dataspace.getSimpleExtentDims( DIMS );
        const hsize_t ROWS = DIMS[0];
        const hsize_t COLS = DIMS[1];

        // just read everything all at once...offsets should always be small
        // (yeah, right!)
        long read[ROWS][COLS] = { };
        offsets.read( read, offsets.getDataType( ) );
        for ( size_t row = 0; row < ROWS; row++ ) {
          info->addOffset( read[row][1], read[row][0] );
        }
      }
    }
    egroup.close( );
  }
  catch ( H5::FileIException error ) {
    output( ) << "/Waveforms: " << error.getDetailMsg( ) << std::endl;
  }
  // catch failure caused by the DataSet operations
  catch ( H5::DataSetIException error ) {
    output( ) << "/Waveforms: " << error.getDetailMsg( ) << std::endl;
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

std::vector<dr_time> Hdf5Reader::readTimes( H5::DataSet & dataset ) const {
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
    signal->setMeta( SignalData::CHUNK_INTERVAL_MS, timeinterval );
  }
  int valsPerChunk = 1;
  if ( dataAndTimeGroup.attrExists( SignalData::READINGS_PER_CHUNK ) ) {
    valsPerChunk = metaint( dataAndTimeGroup, SignalData::READINGS_PER_CHUNK );
    signal->setMeta( SignalData::READINGS_PER_CHUNK, valsPerChunk );
  }

  H5::DataSet dataset = dataAndTimeGroup.openDataSet( "data" );

  copymetas( signal, dataset );
  int scale = signal->scale( );

  H5::DataSet ds = dataAndTimeGroup.openDataSet( "time" );
  std::vector<dr_time> times = readTimes( ds );
  ds.close( );

  if ( iswave ) {
    fillWave( signal, dataset, times, valsPerChunk, scale );
  }
  else {
    fillVital( signal, dataset, times, timeinterval, valsPerChunk, scale );
  }
  dataset.close( );
}

void Hdf5Reader::fillVital( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
    const std::vector<dr_time>& times, int timeinterval, int valsPerTime, int scale ) const {
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

  int powscale = std::pow( 10, scale );
  // just read everything all at once...in the future, we probably want to
  // worry about using hyperslabs
  if ( dataset.getDataType( ) == H5::PredType::STD_I16LE ) {
    short read[ROWS][COLS] = { };
    dataset.read( read, dataset.getDataType( ) );
    for ( size_t row = 0; row < ROWS; row++ ) {
      short val = read[row][0];
      std::string valstr;

      if ( 0 != scale ) {
        valstr = std::to_string( (double) val / powscale );
        // remove any trailing 0s from the string
        while ( '0' == valstr[valstr.size( ) - 1] ) {
          valstr.erase( valstr.size( ) - 1, 1 );
        }
        // make sure we don't end in a .
        if ( '.' == valstr[valstr.size( ) - 1] ) {
          valstr.erase( valstr.size( ) - 1 );
        }
      }
      else {
        valstr = std::to_string( val );
      }

      // FIXME: we better hope valsPerTime is always 1!
      DataRow drow( times[row / valsPerTime], valstr );
      if ( COLS > 1 ) {
        for ( size_t c = 1; c < COLS; c++ ) {
          drow.extras[attrmap[c]] = std::to_string( read[row][c] );
        }
      }
      signal->add( drow );
    }
  }
  else { // data is in integers
    int read[ROWS][COLS] = { };
    dataset.read( read, dataset.getDataType( ) );
    for ( size_t row = 0; row < ROWS; row++ ) {

      int val = read[row][0];
      std::string valstr;

      if ( 0 != scale ) {
        valstr = std::to_string( (double) val / powscale );
        // remove any trailing 0s from the string
        while ( '0' == valstr[valstr.size( ) - 1] ) {
          valstr.erase( valstr.size( ) - 1, 1 );
        }
        // make sure we don't end in a .
        if ( '.' == valstr[valstr.size( ) - 1] ) {
          valstr.erase( valstr.size( ) - 1 );
        }
      }
      else {
        valstr = std::to_string( val );
      }

      // FIXME: we better hope valsPerTime is always 1!
      DataRow drow( times[row / valsPerTime], valstr );
      if ( COLS > 1 ) {
        for ( size_t c = 1; c < COLS; c++ ) {
          drow.extras[attrmap[c]] = std::to_string( read[row][c] );
        }
      }
      signal->add( drow );
    }
  }

  signal->scale( scale );
}

void Hdf5Reader::fillWave( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
    const std::vector<dr_time>& times, int valsPerTime, int scale ) const {
  H5::DataSpace dataspace = dataset.getSpace( );
  hsize_t DIMS[2] = { };
  dataspace.getSimpleExtentDims( DIMS );

  const hsize_t ROWS = DIMS[0];
  const hsize_t COLS = DIMS[1];
  const hsize_t MAXSLABROWS = 128 * 1024;
  hsize_t slabrows = ( MAXSLABROWS > ROWS ? ROWS : MAXSLABROWS );

  hsize_t offset[] = { 0, 0 };
  hsize_t count[] = { slabrows, COLS };
  const hsize_t offset0[] = { 0, 0 };

  std::string values;
  int valcnt = 0;

  std::vector<short> shortbuff;
  std::vector<int> intbuff;

  bool doints = ( H5::PredType::STD_I32LE == dataset.getDataType( ) );
  if ( doints ) {
    intbuff.reserve( slabrows );
  }
  else {
    shortbuff.reserve( slabrows );
  }

  int timecounter = 0;
  int powscale = std::pow( 10, scale );
  while ( offset[0] < ROWS ) {
    dataspace.selectHyperslab( H5S_SELECT_SET, count, offset );
    H5::DataSpace memspace( 2, count );
    memspace.selectHyperslab( H5S_SELECT_SET, count, offset0 );
    if ( doints ) {
      dataset.read( &intbuff[0], dataset.getDataType( ), memspace, dataspace );
    }
    else {
      dataset.read( &shortbuff[0], dataset.getDataType( ), memspace, dataspace );
    }

    for ( size_t row = 0; row < count[0]; row++ ) {
      if ( !values.empty( ) ) {
        values.append( "," );
      }

      std::string valstr;
      int val = ( doints ? intbuff[row] : shortbuff[row] );
      if ( 0 != scale ) {
        valstr = std::to_string( (double) val / powscale );
        // remove any trailing 0s from the string
        while ( '0' == valstr[valstr.size( ) - 1] ) {
          valstr.erase( valstr.size( ) - 1, 1 );
        }
        // make sure we don't end in a .
        if ( '.' == valstr[valstr.size( ) - 1] ) {
          valstr.erase( valstr.size( ) - 1 );
        }
        else {
          valstr = std::to_string( val );
        }
      }
      else {
        short val = shortbuff[row];
        if ( 0 != scale ) {
          valstr = std::to_string( (double) val / powscale );
          // remove any trailing 0s from the string
          while ( '0' == valstr[valstr.size( ) - 1] ) {
            valstr.erase( valstr.size( ) - 1, 1 );
          }
          // make sure we don't end in a .
          if ( '.' == valstr[valstr.size( ) - 1] ) {
            valstr.erase( valstr.size( ) - 1 );
          }
        }
        else {
          valstr = std::to_string( val );
        }
      }

      values.append( valstr );

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
    H5::DataSet & dataset ) const {
  hsize_t cnt = dataset.getNumAttrs( );

  for ( size_t i = 0; i < cnt; i++ ) {
    H5::Attribute attr = dataset.openAttribute( i );
    H5::DataType type = attr.getDataType( );
    const std::string key = attr.getName( );
    if ( 0 == IGNORABLE_PROPS.count( key ) ) {

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
}

int Hdf5Reader::metaint( const H5::H5Location& loc, const std::string & attrname ) {
  int val;
  if ( loc.attrExists( attrname ) ) {

    H5::Attribute attr = loc.openAttribute( attrname );
    attr.read( attr.getDataType( ), &val );
  }

  return val;
}

std::string Hdf5Reader::metastr( const H5::H5Location& loc, const std::string & attrname ) {
  return metastr( loc.openAttribute( attrname ) );
}

std::string Hdf5Reader::metastr( const H5::Attribute & attr ) {
  H5::DataType type = attr.getDataType( );

  std::string aval;
  switch ( attr.getTypeClass( ) ) {
    case H5T_INTEGER:
    {
      long inty = 0;
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

unsigned int Hdf5Reader::layoutVersion( const H5::H5File& file ) {
  unsigned int rev = 0;
  if ( file.attrExists( "Layout Version" ) ) {
    auto attr = file.openAttribute( "Layout Version" );
    std::string attrval = metastr( attr );
    std::istringstream stream( attrval );
    int count = 0;
    for ( std::string each; std::getline( stream, each, '.' ); ) {
      count++;
      try {
        int val = std::stoi( each );
        if ( 1 == count ) {
          rev += 10000 * val;
        }
        else if ( 2 == count ) {
          rev += 100 * val;
        }
        else {
          rev += val;
        }
      }
      catch ( std::invalid_argument x ) {
        // don't care
      }
    }
  }

  return rev;
}

std::unique_ptr<SignalData> Hdf5Reader::splice( const std::string& inputfile,
    const std::string& path, dr_time from, dr_time to ) {

  size_t typeo = path.find( "VitalSigns" );
  std::string signalname = path.substr( path.rfind( "/" ) + 1 );

  std::unique_ptr<SignalData> signal( new BasicSignalData( signalname, std::string::npos == typeo ) );
  H5::Exception::dontPrint( );
  try {
    file = H5::H5File( inputfile, H5F_ACC_RDONLY );
    H5::Group group = file.openGroup( path );
    H5::DataSet times = group.openDataSet( "time" );
    H5::DataSet data = group.openDataSet( "data" );
    H5::DataSet globaltimes = group.openDataSet( "/Events/Global_Times" );

    int readingsperperiod = metaint( data, SignalData::READINGS_PER_CHUNK );
    int periodtime = metaint( data, SignalData::CHUNK_INTERVAL_MS );
    signal->setChunkIntervalAndSampleRate( periodtime, readingsperperiod );
    bool doints = ( H5::PredType::STD_I32LE == data.getDataType( ) );

    bool timeisindex = ( layoutVersion( file ) >= 40100
        ? "index to Global_Times" == metastr( times, "Columns" )
        : false );
    std::vector<dr_time> realtimes;
    bool foundFrom = false;
    bool foundTo = false;
    hsize_t fromidx;
    hsize_t toidx;

    std::map<dr_time, int> values;
    if ( timeisindex ) {
      fromidx = getIndexForTime( globaltimes, from, &foundFrom );
      toidx = getIndexForTime( globaltimes, to, &foundTo );

      // FIXME: now look in times to see which indices we actually want
      // FIXME: none of this works yet
      //realtimes = slabreadt( ( timeisindex ? globaltimes : times ), indexloc1, indexloc2 );
    }
    else {
      fromidx = getIndexForTime( times, from, &foundFrom );
      toidx = getIndexForTime( times, to, &foundTo );
      auto realtimes2( slabreadt( times, fromidx, toidx ) );
      std::cout << "retrieved realtimes" << std::endl;
      for ( auto x : realtimes2 ) {
        std::cout << x << std::endl;
      }
      realtimes = realtimes2;

    }

    auto datavals = ( doints
        ? slabreadi( data, fromidx, toidx )
        : slabreads( data, fromidx, toidx ) );

    for ( auto x : realtimes ) {
      std::cout << x << std::endl;
    }

    for ( size_t i = 0; i < realtimes.size( ); i++ ) {
      values.insert( std::make_pair( realtimes[i], datavals[i] ) );
    }

    for ( auto x : values ) {
      std::cout << x.first << ": " << x.second << std::endl;
    }
  }
  catch ( H5::FileIException error ) {
    output( ) << error.getDetailMsg( ) << std::endl;
    file.close( );
  }
  // catch failure caused by the DataSet operations
  catch ( H5::DataSetIException error ) {
    output( ) << error.getDetailMsg( ) << std::endl;
    file.close( );
  }

  return signal;
}

hsize_t Hdf5Reader::getIndexForTime( H5::DataSet& haystack, dr_time needle, bool * found ) {
  hsize_t DIMS[2] = { };
  H5::DataSpace dsspace = haystack.getSpace( );
  dsspace.getSimpleExtentDims( DIMS );

  const hsize_t ROWS = DIMS[0];
  //const hsize_t COLS = DIMS[1];

  // we'll do a binary search to get our number (or at least close to it!)
  hsize_t startpos = 0;
  hsize_t endpos = ROWS - 1;

  hsize_t dim[] = { 1, 1 };
  hsize_t count[] = { 1, 1 };

  H5::DataSpace searchspace( 2, dim );

  dr_time checktime;
  hsize_t checkpos = 0;
  while ( startpos < endpos ) { // stop looking if we can't find it
    checkpos = ( startpos + endpos ) / 2;

    hsize_t offset[] = { checkpos, 0 };
    dsspace.selectHyperslab( H5S_SELECT_SET, count, offset );
    haystack.read( &checktime, haystack.getDataType( ), searchspace, dsspace );

    if ( checktime > needle ) {
      endpos = checkpos - 1;
    }
    else if ( checktime < needle ) {
      startpos = checkpos + 1;
    }
  }

  if ( nullptr != found ) {
    *found = ( checktime == needle );
  }
  return checkpos;
}

/**
 * Reads the given dataset from start(inclusive) to end (exclusive) as ints
 * @param data
 * @param startidx
 * @param endidx
 * @return
 */
std::vector<int> Hdf5Reader::slabreadi( H5::DataSet& ds, hsize_t startrow, hsize_t endrow ) {
  std::vector<int> values;
  hsize_t rowstoget = endrow - startrow;
  values.reserve( rowstoget );

  hsize_t DIMS[2] = { };
  H5::DataSpace dsspace = ds.getSpace( );
  dsspace.getSimpleExtentDims( DIMS );

  //const hsize_t ROWS = DIMS[0];
  const hsize_t COLS = DIMS[1];

  // we'll get everything in one read
  hsize_t dim[] = { rowstoget, COLS };
  hsize_t count[] = { rowstoget, COLS };

  H5::DataSpace searchspace( 2, dim );

  hsize_t offset[] = { startrow, 0 };
  hsize_t stride[] = { 1, COLS };

  int dd[rowstoget][COLS] = { };
  dsspace.selectHyperslab( H5S_SELECT_SET, count, offset, stride );
  ds.read( &dd, ds.getDataType( ), searchspace, dsspace );

  //  for ( hsize_t i = startidx; i < endidx; i++ ) {
  //    std::cout << "row " << i << ":";
  //    for ( hsize_t j = 0; j < COLS; j++ ) {
  //      std::cout << " " << dd[i][j];
  //    }
  //    std::cout << std::endl;
  //  }

  return values;
}

std::vector<int> Hdf5Reader::slabreads( H5::DataSet& ds, hsize_t startrow, hsize_t endrow ) {
  hsize_t rowstoget = endrow - startrow;

  std::vector<short> values;
  values.reserve( rowstoget );

  hsize_t DIMS[2] = { };
  H5::DataSpace dsspace = ds.getSpace( );
  dsspace.getSimpleExtentDims( DIMS );

  //const hsize_t ROWS = DIMS[0];
  const hsize_t COLS = DIMS[1];
  //std::cout << "reading shorts " << ROWS << "," << COLS << std::endl;

  // we'll get everything in one read
  hsize_t dim[] = { rowstoget, COLS };
  hsize_t count[] = { rowstoget, COLS };

  H5::DataSpace searchspace( 2, dim );

  hsize_t offset[] = { startrow, 0 };
  hsize_t stride[] = { 1, COLS };
  dsspace.selectHyperslab( H5S_SELECT_SET, count, offset, stride );
  ds.read( &values[0], ds.getDataType( ), searchspace, dsspace );

  std::vector<int> ints;
  ints.reserve( values.size( ) );
  for ( size_t i = 0; i < values.size( ); i++ ) {
    ints.push_back( (int) ( values[i] ) );
  }

  //  for ( hsize_t cnt = startidx; cnt < endidx; cnt++ ) {
  //    std::cout << "row " << cnt << ":" << values[cnt - startidx] << std::endl;
  //  }

  return ints;
}

std::vector<dr_time> Hdf5Reader::slabreadt( H5::DataSet& ds, hsize_t startrow, hsize_t endrow ) {
  hsize_t rowstoget = endrow - startrow;

  std::vector<dr_time> values;
  values.reserve( rowstoget );

  hsize_t DIMS[2] = { };
  H5::DataSpace dsspace = ds.getSpace( );
  dsspace.getSimpleExtentDims( DIMS );

  //const hsize_t ROWS = DIMS[0];
  const hsize_t COLS = DIMS[1];
  //std::cout << "reading shorts " << ROWS << "," << COLS << std::endl;

  // we'll get everything in one read
  hsize_t dim[] = { rowstoget, COLS };
  hsize_t count[] = { rowstoget, COLS };

  H5::DataSpace searchspace( 2, dim );

  hsize_t offset[] = { startrow, 0 };
  hsize_t stride[] = { 1, COLS };
  dsspace.selectHyperslab( H5S_SELECT_SET, count, offset, stride );
  ds.read( &values[0], ds.getDataType( ), searchspace, dsspace );

  for ( hsize_t cnt = startrow; cnt < endrow; cnt++ ) {
    std::cout << "row " << cnt << ":" << values[cnt - startrow] << std::endl;
  }

  return values;
}