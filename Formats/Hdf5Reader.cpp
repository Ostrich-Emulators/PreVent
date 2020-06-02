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
#include "SignalUtils.h"
#include "BasicSignalData.h"
#include "OffsetTimeSignalSet.h"

namespace FormatConverter{
  const std::set<std::string> Hdf5Reader::IGNORABLE_PROPS({ "Duration", "End Date/Time",
    "Start Date/Time", SignalData::ENDTIME, SignalData::STARTTIME, SignalData::SCALE, SignalData::MSM,
    "Layout Version", "HDF5 Version", "HDF5 Version", "Layout Version",
    "Columns", SignalData::TIMEZONE, SignalData::LABEL, "Source Reader",
    "Note on Scale", "Note on Min/Max", "Min Value", "Max Value" } );

  Hdf5Reader::Hdf5Reader( ) : Reader( "HDF5" ) { }

  Hdf5Reader::Hdf5Reader( const Hdf5Reader& ) : Reader( "HDF5" ) { }

  Hdf5Reader::~Hdf5Reader( ) { }

  int Hdf5Reader::prepare( const std::string& filename, SignalSet * info ) {
    H5::Exception::dontPrint( );
    int rslt = Reader::prepare( filename, info );
    if ( 0 == rslt ) {
      try {
        file = H5::H5File( filename, H5F_ACC_RDONLY );
      }
      catch ( H5::FileIException& error ) {
        output( ) << error.getDetailMsg( ) << std::endl;
        return -1;
      }
      // catch failure caused by the DataSet operations
      catch ( H5::DataSetIException& error ) {
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
    catch ( H5::FileIException& error ) {
      output( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
      return false;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      output( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
      return false;
    }

    return true;
  }

  bool Hdf5Reader::getAttributes( const std::string& inputfile, const std::string& signal,
      std::map<std::string, int>& mapi, std::map<std::string, double>& mapd, std::map<std::string, std::string>& maps,
      dr_time& starttime, dr_time& endtime ) {
    H5::Exception::dontPrint( );
    try {
      file = H5::H5File( inputfile, H5F_ACC_RDONLY );
      H5::Group data = file.openGroup( signal );
      //H5::DataSet data = root.openDataSet( "data" );

      auto ss = std::unique_ptr<SignalData>{ std::make_unique<BasicSignalData>( "-" ) };
      copymetas( ss.get( ), data, true );

      for ( const auto& a : ss->metas( ) ) {
        maps[a.first] = a.second;
      }
      for ( const auto& a : ss->metai( ) ) {
        mapi[a.first] = a.second;
      }
      for ( const auto& a : ss->metad( ) ) {
        mapd[a.first] = a.second;
      }

      H5::Attribute sattr = data.openAttribute( SignalData::STARTTIME );
      sattr.read( sattr.getDataType( ), &starttime );

      H5::Attribute eattr = data.openAttribute( SignalData::ENDTIME );
      eattr.read( eattr.getDataType( ), &endtime );
    }
    catch ( H5::FileIException& error ) {
      output( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
      return false;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      output( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
      return false;
    }

    return true;
  }

  ReadResult Hdf5Reader::fill( SignalSet * info, const ReadResult& ) {
    H5::Group root = file.openGroup( "/" );

    for ( int i = 0; i < root.getNumAttrs( ); i++ ) {
      H5::Attribute a = root.openAttribute( i );
      auto key = a.getName( );

      if ( 0 == IGNORABLE_PROPS.count( key ) ) {
        std::string prop = ( OffsetTimeSignalSet::COLLECTION_OFFSET == key
            ? std::to_string( metaint( a ) )
            : metastr( a ) );
        info->setMeta( key, prop );
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

      auto auxdata = readAuxData( file );
      for ( auto map : auxdata ) {
        for ( auto& td : map.second ) {
          info->addAuxillaryData( map.first, td );
        }
      }
    }
    catch ( H5::FileIException& error ) {
      output( ) << "/Waveforms: " << error.getDetailMsg( ) << std::endl;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
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
    catch ( H5::FileIException& error ) {
      output( ) << "/VitalSigns: " << error.getDetailMsg( ) << std::endl;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
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
    catch ( H5::FileIException& error ) {
      output( ) << "/Waveforms: " << error.getDetailMsg( ) << std::endl;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      output( ) << "/Waveforms: " << error.getDetailMsg( ) << std::endl;
    }

    return ReadResult::END_OF_FILE;
  }

  std::vector<dr_time> Hdf5Reader::readTimes( H5::DataSet & dataset ) {
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
      times.push_back( modtime( l ) );
    }
    //std::cout << "times vector size is: " << times.size( ) << std::endl;
    //std::cout << "first/last vals: " << times[0] << " " << times[times.size( ) - 1] << std::endl;
    return times;
  }

  void Hdf5Reader::readDataSet( H5::Group& dataAndTimeGroup,
      const bool& iswave, SignalSet * info ) {
    std::string name = metastr( dataAndTimeGroup, SignalData::LABEL );

    auto signal = ( iswave
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

    auto auxdata = readAuxData( dataAndTimeGroup );
    for ( auto map : auxdata ) {
      for ( auto& td : map.second ) {
        signal->addAuxillaryData( map.first, td );
      }
    }

    dataset.close( );
  }

  std::map<std::string, std::vector<TimedData>> Hdf5Reader::readAuxData( H5::Group& auxparent ) {
    std::map<std::string, std::vector < TimedData>> datamap;
    if ( auxparent.exists( "Auxillary_Data" ) ) {
      H5::Group auxdata = auxparent.openGroup( "Auxillary_Data" );


      H5::StrType st( H5::PredType::C_S1, H5T_VARIABLE );
      st.setCset( H5T_CSET_UTF8 );

      for ( size_t i = 0; i < auxdata.getNumObjs( ); i++ ) {
        if ( H5G_GROUP == auxdata.getObjTypeByIdx( i ) ) {
          std::string auxname = auxdata.getObjnameByIdx( i );
          H5::Group dataAndTime = auxdata.openGroup( auxname );

          if ( dataAndTime.exists( "time" ) && dataAndTime.exists( "data" ) ) {
            H5::DataSet timeds = dataAndTime.openDataSet( "time" );
            H5::DataSet dataset = dataAndTime.openDataSet( "data" );

            std::vector<dr_time> times = readTimes( timeds );

            hsize_t DIMS[2] = { };
            dataset.getSpace( ).getSimpleExtentDims( DIMS );
            const hsize_t ROWS = DIMS[0];
            if ( ROWS == times.size( ) ) {
              char ** readdata = (char **) malloc( ROWS * sizeof (char * ) );

              dataset.read( readdata, dataset.getDataType( ) );
              for ( size_t i = 0; i < ROWS; i++ ) {
                datamap[auxname].push_back( TimedData( times[i], std::string( readdata[i] ) ) );
              }
              free( readdata );
            }
            else {
              std::cerr << "\"time\" and \"data\" datasets must have the same number of data points...ignoring" << std::endl;
            }
          }
          else {
            std::cerr << "auxillary data group must have a \"time\" and \"data\" dataset" << std::endl;
          }
        }
      }
    }

    return datamap;
  }

  void Hdf5Reader::fillVital( SignalData * signal, H5::DataSet& dataset,
      const std::vector<dr_time>& times, int timeinterval, int valsPerTime, int scale ) const {
    H5::DataSpace dataspace = dataset.getSpace( );
    hsize_t DIMS[2] = { };
    dataspace.getSimpleExtentDims( DIMS );
    const hsize_t ROWS = DIMS[0];
    const hsize_t COLS = DIMS[1];

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

    // FIXME: use hyperslabs (slabreadi/slabreads is fine when we only have 1 column)

    if ( dataset.getDataType( ) == H5::PredType::STD_I16LE ) {
      short read[ROWS][COLS] = { };

      dataset.read( read, dataset.getDataType( ) );
      for ( size_t row = 0; row < ROWS; row++ ) {
        short val = read[row][0];

        // FIXME: we better hope valsPerTime is always 1!
        auto drow = std::make_unique<DataRow>( times[row / valsPerTime], val, scale );
        if ( COLS > 1 ) {
          for ( size_t c = 1; c < COLS; c++ ) {
            drow->extras[attrmap[c]] = std::to_string( read[row][c] );
          }
        }
        signal->add( std::move( drow ) );
      }
    }
    else { // data is in integers
      int read[ROWS][COLS] = { };
      dataset.read( read, dataset.getDataType( ) );
      for ( size_t row = 0; row < ROWS; row++ ) {

        int val = read[row][0];

        // FIXME: we better hope valsPerTime is always 1!
        auto drow = std::make_unique<DataRow>( times[row / valsPerTime], val, scale );
        if ( COLS > 1 ) {
          for ( size_t c = 1; c < COLS; c++ ) {
            drow->extras[attrmap[c]] = std::to_string( read[row][c] );
          }
        }
        signal->add( std::move( drow ) );
      }
    }

    signal->scale( scale );
  }

  void Hdf5Reader::fillWave( SignalData * signal, H5::DataSet& dataset,
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

    std::vector<int> values;
    values.reserve( valsPerTime );

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
        int val = ( doints ? intbuff[row] : shortbuff[row] );
        values.push_back( val );

        if ( static_cast<size_t> ( valsPerTime ) == values.size( ) ) {
          signal->add( std::make_unique<DataRow>( times[timecounter++], values, scale ) );
          values.clear( );
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

  void Hdf5Reader::copymetas( SignalData * signal,
      H5::H5Object & dataset, bool includeIgnorables ) {
    hsize_t cnt = dataset.getNumAttrs( );

    for ( size_t i = 0; i < cnt; i++ ) {
      H5::Attribute attr = dataset.openAttribute( i );
      H5::DataType type = attr.getDataType( );
      const std::string key = attr.getName( );
      if ( 0 == IGNORABLE_PROPS.count( key ) || includeIgnorables ) {

        switch ( attr.getTypeClass( ) ) {
          case H5T_INTEGER:
          {
            if ( type.getSize( ) <= sizeof ( int ) ) {
              int inty = 0;
              attr.read( type, &inty );
              signal->setMeta( key, inty );
            }
            else if ( type.getSize( ) <= sizeof ( long ) ) {
              std::cerr << "long meta copy not implemented" << std::endl;
              // long inty = 0;
              // attr.read( type, &inty );
              // signal->setMeta( key, inty );
            }
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

  int Hdf5Reader::metaint( const H5::H5Object& loc, const std::string & attrname ) {
    return metaint( loc.openAttribute( attrname ) );
  }

  int Hdf5Reader::metaint( const H5::Attribute & attr ) {
    int val;
    attr.read( attr.getDataType( ), &val );
    return val;
  }

  std::string Hdf5Reader::metastr( const H5::H5Object& loc, const std::string & attrname ) {
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
        catch ( std::invalid_argument& x ) {
          // don't care
        }
      }
    }

    return rev;
  }

  void Hdf5Reader::splice( const std::string& inputfile, const std::string& path,
      dr_time from, dr_time to, SignalData * signal ) {
    size_t typeo = path.find( "VitalSigns" );

    signal->setWave( std::string::npos == typeo );
    H5::Exception::dontPrint( );
    try {
      file = H5::H5File( inputfile, H5F_ACC_RDONLY );
      H5::Group group = file.openGroup( path );
      H5::DataSet times = group.openDataSet( "time" );
      H5::DataSet data = group.openDataSet( "data" );
      H5::DataSet globaltimes = group.openDataSet( "/Events/Global_Times" );

      const int scale = metaint( data, SignalData::SCALE );

      const int readingsperperiod = metaint( data, SignalData::READINGS_PER_CHUNK );
      const int periodtime = metaint( data, SignalData::CHUNK_INTERVAL_MS );
      signal->setChunkIntervalAndSampleRate( periodtime, readingsperperiod );
      signal->scale( scale );
      const bool doints = ( H5::PredType::STD_I32LE == data.getDataType( ) );

      const bool timeisindex = ( layoutVersion( file ) >= 40100
          ? "index to Global_Times" == metastr( times, "Columns" )
          : false );
      std::unique_ptr<TimeRange> realtimes;
      dr_time foundFrom = false;
      dr_time foundTo = false;
      hsize_t fromidx;
      hsize_t toidx;

      if ( timeisindex ) {
        fromidx = getIndexForTime( globaltimes, from, &foundFrom );
        toidx = getIndexForTime( globaltimes, to, &foundTo );
        realtimes = slabreadt( globaltimes, fromidx, toidx );

        // we fall through this block, and now we just want the indexes that
        // match these values
        from = fromidx;
        to = toidx;
      }

      fromidx = getIndexForTime( times, from, &foundFrom );
      toidx = getIndexForTime( times, to, &foundTo );
      //output( ) << from << "\tidx: " << fromidx << "\tfound? " << foundFrom << std::endl;
      //output( ) << to << "\tidx: " << toidx << "\tfound? " << foundTo << std::endl;
      if ( !timeisindex ) {
        realtimes = slabreadt( times, fromidx, toidx );
      }

      // if we have a lot of data to read, we need to split it up into
      // manageable chunks or we'll run out of memory.
      // remember: wave signals have multiple values per time period
      const hsize_t MAXSLABSIZE = 512 * 1024;
      hsize_t slabstartidx = fromidx * readingsperperiod;
      const hsize_t slabstopidx = toidx * readingsperperiod;
      hsize_t currentstopidx = ( slabstopidx - slabstartidx > MAXSLABSIZE
          ? slabstartidx + MAXSLABSIZE
          : slabstopidx );

      hsize_t dataidx = 0;
      std::vector<int> datavals;
      for ( size_t i = 0; i < ( toidx - fromidx ); i++ ) {
        dr_time time = realtimes->next( );

        // if we can't process a whole sample, get more data to process
        if ( dataidx + readingsperperiod > datavals.size( ) ) {
          //        std::cout << "start/stop/cur idx: " << slabstartidx << "/" << slabstopidx
          //            << "/" << currentstopidx << std::endl;

          auto newvals = ( doints
              ? slabreadi( data, slabstartidx, currentstopidx )
              : slabreads( data, slabstartidx, currentstopidx ) );

          // get rid of the stuff we've already processed
          datavals.erase( datavals.begin( ), datavals.begin( ) + dataidx );

          // add the new stuff
          datavals.insert( datavals.end( ), newvals.begin( ), newvals.end( ) );

          // start counting from the beginning again
          dataidx = 0;

          // now get ready for the next time we have to do this
          slabstartidx = currentstopidx;
          currentstopidx = ( slabstopidx - currentstopidx > MAXSLABSIZE
              ? currentstopidx + MAXSLABSIZE
              : slabstopidx );
        }

        if ( signal->wave( ) ) {
          std::vector<int> onerowdata( &datavals[dataidx], &datavals[dataidx + readingsperperiod] );
          signal->add( std::make_unique<DataRow>( time, onerowdata, scale ) );
          dataidx += readingsperperiod;
        }
        else {
          signal->add( std::make_unique<DataRow>( time, datavals[dataidx++], scale ) );
        }
      }
    }
    catch ( H5::FileIException& error ) {
      output( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      output( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
    }
  }

  dr_time Hdf5Reader::getTimeAtIndex( H5::DataSet& haystack, hsize_t index ) {
    hsize_t DIMS[2] = { };
    H5::DataSpace dsspace = haystack.getSpace( );
    dsspace.getSimpleExtentDims( DIMS );

    if ( DIMS[0] < index ) {
      // looking for an index bigger than our dataset
      return std::numeric_limits<dr_time>::max( );
    }

    hsize_t dim[] = { 1, 1 };
    hsize_t count[] = { 1, 1 };

    H5::DataSpace searchspace( 2, dim );

    hsize_t offset[] = { index, 0 };
    dsspace.selectHyperslab( H5S_SELECT_SET, count, offset );
    dr_time checktime;
    haystack.read( &checktime, haystack.getDataType( ), searchspace, dsspace );
    return checktime;
  }

  hsize_t Hdf5Reader::getIndexForTime( H5::DataSet& haystack, dr_time needle, dr_time * found ) {

    hsize_t DIMS[2] = { };
    H5::DataSpace dsspace = haystack.getSpace( );
    dsspace.getSimpleExtentDims( DIMS );

    const hsize_t ROWS = DIMS[0];

    // we'll do a binary search to get our number (or at least close to it!)
    hsize_t startpos = 0;
    hsize_t endpos = ROWS - 1;

    while ( startpos <= endpos ) { // stop looking if we can't find it
      hsize_t checkpos = startpos + ( endpos - startpos ) / 2;
      dr_time checktime = getTimeAtIndex( haystack, checkpos );
      // std::cout << "timesearch: " << startpos << "-" << endpos << "=>"
      //   << checkpos << " => " << checktime << std::endl;

      if ( startpos == endpos && checktime != needle ) {
        // no where else to look, but we didn't find our needle

        if ( checktime < needle && checkpos < ROWS - 1 ) {
          checkpos++;
        }
        if ( nullptr != found ) {
          *found = getTimeAtIndex( haystack, checkpos );
        }

        if ( checkpos == endpos ) {
          checkpos++;
        }
        return checkpos;
      }

      if ( checktime == needle ) { // we found the time we want!
        if ( nullptr != found ) {
          *found = checktime;
        }
        return checkpos;
      }
      else if ( checktime < needle ) { // the time we found is too small
        if ( checkpos < ROWS ) {
          startpos = checkpos + 1;
        }
        else {
          // didn't find the needle, but we're out of places to look
          if ( nullptr != found ) {
            *found = checktime;
          }
          return checkpos;
        }
      }
      else { // the time we found is too big
        if ( checkpos > 0 ) {
          endpos = checkpos - 1;
        }
        else {
          // didn't find the needle, but we're out of places to look
          if ( nullptr != found ) {
            *found = checktime;
          }
          return checkpos;
        }
      }
    }

    return 0; // should never get here
  }

  /**
   * Reads the given dataset from start(inclusive) to end (exclusive) as ints
   * @param data
   * @param startidx
   * @param endidx
   * @return
   */
  std::vector<int> Hdf5Reader::slabreadi( H5::DataSet& ds, hsize_t startrow, hsize_t endrow ) {
    hsize_t rowstoget = endrow - startrow;

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

    int dd[rowstoget][COLS] = { };
    dsspace.selectHyperslab( H5S_SELECT_SET, count, offset );
    ds.read( &dd, ds.getDataType( ), searchspace, dsspace );

    std::vector<int> values( rowstoget );
    for ( hsize_t i = 0; i < rowstoget; i++ ) {
      values[i] = dd[i][0];
    }

    return values;
  }

  std::vector<int> Hdf5Reader::slabreads( H5::DataSet& ds, hsize_t startrow, hsize_t endrow ) {
    const hsize_t rowstoget = endrow - startrow;

    hsize_t DIMS[2] = { };
    H5::DataSpace dsspace = ds.getSpace( );
    dsspace.getSimpleExtentDims( DIMS );

    //const hsize_t ROWS = DIMS[0];
    const hsize_t COLS = DIMS[1];
    //std::cout << "reading shorts " << ROWS << "," << COLS << std::endl;

    // we'll get everything in one read
    const hsize_t dim[] = { rowstoget, COLS };
    const hsize_t count[] = { rowstoget, COLS };

    H5::DataSpace searchspace( 2, dim );

    const hsize_t offset[] = { startrow, 0 };
    short dd[rowstoget][COLS] = { };
    dsspace.selectHyperslab( H5S_SELECT_SET, count, offset );
    ds.read( &dd, ds.getDataType( ), searchspace, dsspace );

    std::vector<int> values( rowstoget );
    for ( hsize_t i = 0; i < rowstoget; i++ ) {
      values[i] = static_cast<int> ( dd[i][0] );
    }

    return values;
  }

  std::vector<dr_time> Hdf5Reader::slabreadt_small( H5::DataSet& ds, hsize_t startrow, hsize_t endrow ) {
    const hsize_t rowstoget = endrow - startrow;

    const auto MAX_GET = 1024 * 512;
    if ( rowstoget > MAX_GET ) {
      throw new std::runtime_error( "too many times requested" );
    }


    hsize_t DIMS[2] = { };
    H5::DataSpace dsspace = ds.getSpace( );
    dsspace.getSimpleExtentDims( DIMS );

    //const hsize_t ROWS = DIMS[0];
    const hsize_t COLS = DIMS[1];
    //std::cout << "reading shorts " << ROWS << "," << COLS << std::endl;

    // we'll get everything in one read
    const hsize_t dim[] = { rowstoget, COLS };
    const hsize_t count[] = { rowstoget, COLS };

    H5::DataSpace searchspace( 2, dim );

    const hsize_t offset[] = { startrow, 0 };
    const hsize_t stride[] = { 1, COLS };
    dr_time times[rowstoget];
    dsspace.selectHyperslab( H5S_SELECT_SET, count, offset, stride );
    ds.read( &times, ds.getDataType( ), searchspace, dsspace );

    const int sizer = sizeof ( times ) / sizeof ( times[0] );
    std::vector<dr_time> values( times, times + sizer );

    return values;
  }

  std::unique_ptr<TimeRange> Hdf5Reader::slabreadt( H5::DataSet& ds, hsize_t startrow, hsize_t endrow ) {
    FILE * cache = tmpfile( );
    auto range = std::make_unique<TimeRange>( startrow, endrow, cache );

    const auto MAX_GET = 1024 * 512;
    auto start2 = startrow;
    while ( start2 < endrow ) {
      auto end2 = start2 + MAX_GET;
      if ( end2 > endrow ) {
        end2 = endrow;
      }

      auto tmprows = slabreadt_small( ds, start2, end2 );
      std::fwrite( tmprows.data( ), sizeof ( dr_time ), tmprows.size( ), cache );
      start2 = end2;
    }

    std::rewind( cache );
    return range;
  }
}