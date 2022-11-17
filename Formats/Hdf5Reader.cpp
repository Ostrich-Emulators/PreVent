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
#include "Log.h"
#include "FileNamer.h"
#include "TimeParser.h"

namespace FormatConverter{
  const std::set<std::string> Hdf5Reader::IGNORABLE_PROPS{ "Duration", "End Date/Time",
    "Start Date/Time", "Scale",
    "Missing Value Marker", "Layout Version", "HDF5 Version", "HDF5 Version", "Layout Version",
    "Columns", "Timezone", "Data Label", "Source Reader",
    "Note on Scale", "Note on Min/Max", "Min Value", "Max Value", "Start Time", "End Time" };

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
        Log::error( ) << error.getDetailMsg( ) << std::endl;
        return -1;
      }
      // catch failure caused by the DataSet operations
      catch ( H5::DataSetIException& error ) {
        Log::error( ) << error.getDetailMsg( ) << std::endl;
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
      Log::error( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
      return false;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      Log::error( ) << error.getDetailMsg( ) << std::endl;
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
      Log::error( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
      return false;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      Log::error( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
      return false;
    }

    return true;
  }

  ReadResult Hdf5Reader::fill( SignalSet * info, const ReadResult& lastread ) {
    H5::Group root = file.openGroup( "/" );

    if ( ReadResult::FIRST_READ == lastread ) {
      trackers.clear( );
    }

    for ( int i = 0; i < root.getNumAttrs( ); i++ ) {
      H5::Attribute a = root.openAttribute( i );
      auto key = a.getName( );

      if ( IGNORABLE_PROPS.end( ) == IGNORABLE_PROPS.find( key ) ) {
        std::string prop = ( OffsetTimeSignalSet::COLLECTION_OFFSET == key
            ? std::to_string( metaint( a ) )
            : metastr( a ) );

        Log::debug( ) << "read attr: " << key << ": " << prop << std::endl;
        info->setMeta( key, prop );
      }
      else {
        Log::trace( ) << "skipping attr: " << key << std::endl;
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
          auto read = std::vector<long>( ROWS * COLS );
          offsets.read( read.data( ), offsets.getDataType( ) );
          for ( size_t row = 0; row < ROWS; row++ ) {
            info->addOffset( read[2 * row + 1], read[2 * row] );
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
      Log::error( ) << "/: " << error.getDetailMsg( ) << std::endl;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      Log::error( ) << "/: " << error.getDetailMsg( ) << std::endl;
    }

    try {
      H5::Group vgroup = file.openGroup( "/VitalSigns" );
      for ( size_t i = 0; i < vgroup.getNumObjs( ); i++ ) {
        std::string vital = vgroup.getObjnameByIdx( i );
        H5::Group dataAndTimeGroup = vgroup.openGroup( vital );
        initSignalAndTracker( dataAndTimeGroup, false, info );
      }
      vgroup.close( );
    }
    catch ( H5::FileIException& error ) {
      Log::error( ) << "/VitalSigns: " << error.getDetailMsg( ) << std::endl;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      Log::error( ) << "/VitalSigns: " << error.getDetailMsg( ) << std::endl;
    }

    if ( !this->skipwaves( ) ) {
      try {
        H5::Group wgroup = file.openGroup( "/Waveforms" );
        for ( size_t i = 0; i < wgroup.getNumObjs( ); i++ ) {
          std::string wave = wgroup.getObjnameByIdx( i );
          H5::Group dataAndTimeGroup = wgroup.openGroup( wave );
          initSignalAndTracker( dataAndTimeGroup, true, info );
        }
        wgroup.close( );
      }
      catch ( H5::FileIException& error ) {
        Log::error( ) << "/Waveforms: " << error.getDetailMsg( ) << std::endl;
      }
      // catch failure caused by the DataSet operations
      catch ( H5::DataSetIException& error ) {
        Log::error( ) << "/Waveforms: " << error.getDetailMsg( ) << std::endl;
      }
    }

    fillOneDuration( info );

    auto hasmore = false;
    for ( auto& x : trackers ) {
      if ( !x.second.done( ) ) {
        hasmore = true;
      }
    }

    return (hasmore ? ReadResult::END_OF_DURATION
        : ReadResult::END_OF_FILE );
  }

  void Hdf5Reader::fillOneDuration( SignalSet * info ) {
    dr_time earliest = std::numeric_limits<dr_time>::max( );
    dr_time latest = std::numeric_limits<dr_time>::min( );
    // get the earliest date from all the trackers
    for ( const auto& t : trackers ) {
      Log::warn( ) << std::setw( 20 ) << t.first << " current time: "
          << TimeParser::format( t.second.currtime( ) )
          << ( t.second.done( ) ? "\tDONE" : "\tnot done" ) << std::endl;
      if ( t.second.currtime( ) < earliest && !t.second.done( ) ) {
        earliest = t.second.currtime( );
      }
      if ( *t.second.times.rbegin( ) > latest ) {
        latest = *t.second.times.rbegin( );
      }
    }

    // increment the time until we get a rollover (increment per second for simplicity)
    auto rolltime = earliest + 1000;

    while ( rolltime < latest &&
        !this->splitter( ).isRollover( earliest, rolltime, this->localizingTime( ) ) ) {
      rolltime += 1000;
    }

    Log::warn( ) << "duration limits: "
        << TimeParser::format( earliest ) << " to " << TimeParser::format( rolltime )
        << std::endl;

    // we now have our window to fill, so cycle through the data sets to fill info
    for ( auto& entry : trackers ) {
      auto& tracker = entry.second;
      if ( tracker.done( ) ) {
        continue;
      }

      if ( tracker.currtime( ) < rolltime && !tracker.done( ) ) {
        Log::warn( ) << std::setw( 20 ) << tracker.path << "\t"
            << TimeParser::format( tracker.currtime( ) )
            << " (" << ( rolltime - tracker.currtime( ) ) / 1000 << "s)\t"
            << ( tracker.done( ) ? "DONE" : "not done" )
            << "\tincluded" << std::endl;
        try {
          H5::Group dataAndTimeGroup = file.openGroup( tracker.path );
          // Log::warn( ) << "filling wave:" << tracker.label << std::endl;
          readDataSet( dataAndTimeGroup, tracker, info );
          dataAndTimeGroup.close( );
        }
        catch ( H5::FileIException& error ) {
          Log::error( ) << tracker.path << " " << error.getDetailMsg( ) << std::endl;
        }
      }
      // else {
      //   Log::warn( ) << std::setw( 20 ) << tracker.path << "\t"
      //       << TimeParser::format( tracker.currtime( ) )
      //       << " (" << ( rolltime - tracker.currtime( ) ) / 1000 << "s)\t"
      //       << ( tracker.done( ) ? "DONE" : "not done" )
      //       << "\tNOT INCLUDED" << std::endl;

      // }
    }

    auto removers = std::vector<std::string>{ };
    for ( auto& entry : trackers ) {
      if ( entry.second.done( ) ) {
        removers.push_back( entry.first );
      }
    }
    for ( const auto& x : removers ) {
      trackers.erase( x );
    }
  }

  std::vector<dr_time> Hdf5Reader::readTimes( H5::DataSet & dataset ) {
    //std::cout << group.getObjName( ) << " " << name << std::endl;
    H5::DataSpace dataspace = dataset.getSpace( );
    hsize_t DIMS[2] = { };
    dataspace.getSimpleExtentDims( DIMS );
    const hsize_t ROWS = DIMS[0];
    const hsize_t COLS = DIMS[1];

    const hsize_t sizer = ROWS * COLS;
    auto read = std::vector<long>( sizer );
    dataset.read( read.data( ), dataset.getDataType( ) );
    std::vector<dr_time> times;
    times.reserve( sizer );
    for ( hsize_t i = 0; i < sizer; i++ ) {
      long l = read[i];
      times.push_back( modtime( l ) );
    }
    Log::trace( ) << "times vector size is: " << times.size( ) << std::endl;
    return times;
  }

  std::string Hdf5Reader::initSignalAndTracker( H5::Group& dataAndTimeGroup, const bool& iswave,
      SignalSet * info ) {
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

    const auto path = dataAndTimeGroup.getObjName( );
    const auto label = metastr( dataAndTimeGroup, SignalData::LABEL );
    const auto keyname = SignalTracker::keyname( label, iswave );
    H5::DataSet dataset = dataAndTimeGroup.openDataSet( "data" );

    copymetas( signal, dataset );

    H5::DataSet ds = dataAndTimeGroup.openDataSet( "time" );
    std::vector<dr_time> times = readTimes( ds );
    ds.close( );

    if ( 0 == trackers.count( keyname ) ) {
      trackers.insert( std::make_pair( keyname, Hdf5Reader::SignalTracker( path, label, iswave,
          times ) ) );
    }
    else {
      auto t = trackers.at( keyname ).times;
      t.insert( t.end( ), times.begin( ), times.end( ) );
    }

    return keyname;
  }

  void Hdf5Reader::readDataSet( H5::Group& dataAndTimeGroup,
      SignalTracker& saver, SignalSet * info ) {
    std::string name = metastr( dataAndTimeGroup, SignalData::LABEL );

    auto signal = ( saver.wave
        ? info->addWave( name )
        : info->addVital( name ) );
    H5::DataSet dataset = dataAndTimeGroup.openDataSet( "data" );

    const auto valsPerChunk = signal->metai( ).at( SignalData::READINGS_PER_CHUNK );
    const auto scale = signal->scale( );
    int timeinterval = dataAndTimeGroup.attrExists( SignalData::CHUNK_INTERVAL_MS )
        ? metaint( dataAndTimeGroup, SignalData::CHUNK_INTERVAL_MS )
        : 2000;

    if ( saver.wave ) {
      fillWave( signal, info, dataset, saver, valsPerChunk, scale );
    }
    else {
      fillVital( signal, info, dataset, saver, timeinterval, valsPerChunk, scale );
    }

    auto auxdata = readAuxData( dataAndTimeGroup );
    for ( auto map : auxdata ) {
      for ( auto& td : map.second ) {
        signal->addAuxillaryData( map.first, td );
      }
    }

    dataset.close( );
  }

  std::map<std::string, std::vector < TimedData >> Hdf5Reader::readAuxData( H5::Group & auxparent ) {
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
              Log::warn( ) << "\"time\" and \"data\" datasets must have the same number of data points...ignoring" << std::endl;
            }
          }
          else {
            Log::warn( ) << "auxillary data group must have a \"time\" and \"data\" dataset" << std::endl;
          }
        }
      }
    }

    return datamap;
  }

  void Hdf5Reader::fillVital( SignalData * signal, SignalSet * info, H5::DataSet& dataset,
      SignalTracker& saver, int timeinterval, int valsPerTime, int scale ) const {
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

    auto intread = std::vector<int>{ };
    slabfill( dataset, 0, ROWS, intread );

    while ( saver.timeidx < ROWS && !isRollover( saver.times[saver.timeidx], info ) ) {
      // we can only have 1 valPerTime because this is a vital (otherwise, we'd call it a wave)
      auto rowtime = saver.times[saver.timeidx];
      if ( isRollover( rowtime, info ) ) {
        return;
      }

      int val = intread[COLS * saver.timeidx];
      auto drow = std::make_unique<DataRow>( rowtime, val, scale );
      if ( COLS > 1 ) {
        for ( int c = 1; c < (int) COLS; c++ ) {
          drow->extras[attrmap[c]] = std::to_string( intread[COLS * saver.timeidx + c] );
        }
      }
      signal->add( std::move( drow ) );
      saver.timeidx++;
    }
    signal->scale( scale );
  }

  void Hdf5Reader::fillWave( SignalData * signal, SignalSet * info, H5::DataSet& dataset,
      SignalTracker& saver, int valsPerTime, int scale ) const {
    H5::DataSpace dataspace = dataset.getSpace( );
    hsize_t DIMS[2] = { };
    dataspace.getSimpleExtentDims( DIMS );

    const hsize_t ROWS = DIMS[0];

    hsize_t offset = saver.timeidx * valsPerTime;

    while ( offset < ROWS && !isRollover( saver.times[saver.timeidx], info ) ) {
      std::vector<int> buffer;
      auto startrow = offset;
      auto endrow = startrow + valsPerTime;
      slabfill( dataset, startrow, endrow, buffer );

      signal->add( std::make_unique<DataRow>( saver.times[saver.timeidx++], buffer, scale ) );

      // get ready for the next read
      offset += valsPerTime;
    }
  }

  void Hdf5Reader::copymetas( SignalData * signal,
      H5::H5Object & dataset, bool includeIgnorables ) {
    hsize_t cnt = dataset.getNumAttrs( );

    for ( unsigned int i = 0; i < cnt; i++ ) {
      H5::Attribute attr = dataset.openAttribute( i );
      H5::DataType type = attr.getDataType( );
      const std::string key = attr.getName( );
      if ( IGNORABLE_PROPS.end( ) == IGNORABLE_PROPS.find( key ) || includeIgnorables ) {
        switch ( attr.getTypeClass( ) ) {
          case H5T_INTEGER:
          {
            if ( type.getSize( ) <= sizeof ( int ) ) {
              int inty = 0;
              attr.read( type, &inty );
              signal->setMeta( key, inty );
            }
            else if ( type.getSize( ) <= sizeof ( long ) ) {
              Log::warn( ) << "long meta copy not implemented" << std::endl;
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

  unsigned int Hdf5Reader::layoutVersion( const H5::H5File & file ) {
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

  bool Hdf5Reader::splice( const std::string& inputfile, const std::string& path,
      dr_time from, dr_time to, SignalData * signal ) {
    Log::debug( ) << "splicing data from " << inputfile << ":" << path <<
        " from " << from << " to " << to << std::endl;

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

      const bool timeisindex = ( layoutVersion( file ) >= 40100
          ? "index to Global_Times" == metastr( times, "Columns" )
          : false );
      std::unique_ptr<TimeRange> realtimes;
      hsize_t fromidx;
      hsize_t toidx;

      if ( timeisindex ) {
        fromidx = getIndexForTime( globaltimes, from, true );
        toidx = getIndexForTime( globaltimes, to, false );
        realtimes = slabreadt( globaltimes, fromidx, toidx );

        // we fall through this block, and now we just want the indexes that
        // match these values
        from = fromidx;
        to = toidx;
      }

      fromidx = getIndexForTime( times, from, true );
      toidx = getIndexForTime( times, to, false );
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
      for ( auto time : *realtimes ) {
        // if we can't process a whole sample, get more data to process
        if ( dataidx + readingsperperiod > datavals.size( ) ) {
          Log::trace( ) << "start/stop/cur idx: " << slabstartidx << "/" << slabstopidx
              << "/" << currentstopidx << std::endl;

          // get rid of the stuff we've already processed
          datavals.erase( datavals.begin( ), datavals.begin( ) + dataidx );

          Hdf5Reader::slabfill( data, slabstartidx, currentstopidx, datavals );

          // start counting from the beginning again
          dataidx = 0;

          // now get ready for the next time we have to do this
          slabstartidx = currentstopidx;
          currentstopidx = ( slabstopidx - currentstopidx > MAXSLABSIZE
              ? currentstopidx + MAXSLABSIZE
              : slabstopidx );
        }

        if ( !datavals.empty( ) ) {
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
      return true;
    }
    catch ( H5::FileIException& error ) {
      Log::error( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
      return false;
    }
    // catch failure caused by the DataSet operations
    catch ( H5::DataSetIException& error ) {
      Log::error( ) << error.getDetailMsg( ) << std::endl;
      file.close( );
      return false;
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

  hsize_t Hdf5Reader::getIndexForTime( H5::DataSet& haystack, dr_time needle, bool leftmost ) {

    hsize_t DIMS[2] = { };
    H5::DataSpace dsspace = haystack.getSpace( );
    dsspace.getSimpleExtentDims( DIMS );

    const hsize_t ROWS = DIMS[0];

    // we'll do a binary search to get our number (or at least close to it!)
    hsize_t startpos = 0;
    hsize_t endpos = ROWS;

    while ( startpos < endpos ) {
      auto checkpos = static_cast<hsize_t> ( std::floor( ( endpos + startpos ) / 2.0 ) );
      auto checktime = getTimeAtIndex( haystack, checkpos );

      if ( leftmost ) {
        if ( checktime < needle ) {
          startpos = checkpos + 1;
        }
        else {
          endpos = checkpos;
        }
      }
      else {
        if ( checktime > needle ) {
          endpos = checkpos;
        }
        else {
          startpos = checkpos + 1;
        }
      }
    }

    return ( leftmost
        ? startpos
        : endpos );
  }

  std::vector<int>& Hdf5Reader::slabfill( H5::DataSet& ds, hsize_t startrow, hsize_t endrow, std::vector<int>& target ) {
    hsize_t rowstoget = endrow - startrow;

    hsize_t DIMS[2] = { };
    H5::DataSpace dsspace = ds.getSpace( );
    dsspace.getSimpleExtentDims( DIMS );

    const hsize_t COLS = DIMS[1];

    auto tsz = target.size( );
    target.reserve( tsz + rowstoget * COLS );
    if ( H5::PredType::STD_I16LE == ds.getDataType( ) ) {
      auto shortbuff = std::vector<short>( rowstoget * COLS );
      _slabfill( ds, startrow, endrow, COLS, shortbuff.data( ) );
      target.insert( target.end( ), shortbuff.begin( ), shortbuff.end( ) );
    }
    else {
      _slabfill( ds, startrow, endrow, COLS, target.data( ) + tsz );
    }

    return target;
  }

  void Hdf5Reader::_slabfill( H5::DataSet& ds, hsize_t startrow, hsize_t endrow, hsize_t COLS, void * buffer ) {
    const hsize_t rowstoget = endrow - startrow;

    const hsize_t dim[] = { rowstoget, COLS };
    const hsize_t count[] = { rowstoget, COLS };

    H5::DataSpace dsspace = ds.getSpace( );
    H5::DataSpace searchspace( 2, dim );

    const hsize_t offset[] = { startrow, 0 };

    dsspace.selectHyperslab( H5S_SELECT_SET, count, offset );
    ds.read( buffer, ds.getDataType( ), searchspace, dsspace );
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
    auto times = std::vector<dr_time>( rowstoget );
    dsspace.selectHyperslab( H5S_SELECT_SET, count, offset, stride );
    ds.read( times.data( ), ds.getDataType( ), searchspace, dsspace );

    return times;
  }

  std::unique_ptr<TimeRange> Hdf5Reader::slabreadt( H5::DataSet& ds, hsize_t startrow, hsize_t endrow ) {
    auto range = std::make_unique <TimeRange>( );

    const auto MAX_GET = 1024 * 512;
    auto start2 = startrow;
    while ( start2 < endrow ) {
      auto end2 = start2 + MAX_GET;
      if ( end2 > endrow ) {
        end2 = endrow;
      }

      auto tmprows = slabreadt_small( ds, start2, end2 );
      range->push_back( tmprows );
      start2 = end2;
    }

    return range;
  }

  Hdf5Reader::SignalTracker::SignalTracker( const std::string& path, const std::string& lbl, bool w,
      std::vector<dr_time> _times, hsize_t lastrow ) : wave( w ), times( _times ),
      timeidx( lastrow ), label( lbl ), path( path ) { }

  bool Hdf5Reader::SignalTracker::done( ) const {
    return ( timeidx >= times.size( ) );
  }

  dr_time Hdf5Reader::SignalTracker::currtime( ) const {
    return times[done( ) ? times.size( ) - 1 : timeidx];
  }

  std::string Hdf5Reader::SignalTracker::keyname( const std::string& s, bool wave ) {
    return (wave ? "wave-" : "vital-" )+s;
  }
}