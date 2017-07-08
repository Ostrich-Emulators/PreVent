
#include "FromReader.h"
#include "DataSetDataCache.h"
#include "WfdbReader.h"

FromReader::FromReader( ) : largefile( false ) {
}

FromReader::FromReader( const FromReader& ) {

}

FromReader::~FromReader( ) {
}

std::unique_ptr<FromReader> FromReader::get( const Format& fmt ) {
  switch ( fmt ) {
    case WFDB:
      return std::unique_ptr<FromReader>( new WfdbReader( ) );
  }
}

void FromReader::addVital( const std::string& name, const DataRow& data, const std::string& uom ) {
  if ( 0 == vmap.count( name ) ) {
    vmap.insert( std::make_pair( name,
        std::unique_ptr<DataSetDataCache>( new DataSetDataCache( name,
        largefile ) ) ) );
    vmap[name]->setUom( uom );
  }

  vmap[name]->add( data );
}

void FromReader::addWave( const std::string& name, const DataRow& data, const std::string& uom ) {
  if ( 0 == wmap.count( name ) ) {
    wmap.insert( std::make_pair( name,
        std::unique_ptr<DataSetDataCache>( new DataSetDataCache( name,
        largefile ) ) ) );
  }

  wmap[name]->add( data );
}

void FromReader::reset( const std::string& input ) {
  if ( "-" == input || "-zl" == input ) {
    largefile = false;
  }
  else {
    // arbitrary: "large file" is anything over 750M
    largefile = ( getSize( input ) > 1024 * 1024 * 750 );
  }

  vmap.clear( );
  wmap.clear( );

  doRead( input );
}

std::map<std::string, std::unique_ptr<DataSetDataCache>>&FromReader::vitals( ) {
  return vmap;
}

std::map<std::string, std::unique_ptr<DataSetDataCache>>&FromReader::waves( ) {
  return wmap;
}
