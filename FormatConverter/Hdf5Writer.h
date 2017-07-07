/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Hdf5Writer.h
 * Author: ryan
 *
 * Created on August 26, 2016, 12:58 PM
 */

#ifndef HDF5WRITER_H
#define HDF5WRITER_H

#include <H5Cpp.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <memory>
#include <ctime>

class DataSetDataCache;

class Hdf5Writer {
public:
  virtual ~Hdf5Writer( );

  static int flush( const std::string& outputdir, const std::string& prefix, int compression, 
          const time_t& firstTime, const time_t& lastTime, int ordinal,
          const std::map<std::string, std::string>& datasetattrs,
          std::map<std::string, std::unique_ptr<DataSetDataCache>>& vitals,
          std::map<std::string, std::unique_ptr<DataSetDataCache>>& waves );

  static const int MISSING_VALUE;
private:
  Hdf5Writer( );
  Hdf5Writer( const Hdf5Writer& orig );

  static void writeAttributes( H5::H5File file, std::map<std::string, std::string> datasetattrs,
          const time_t& firstTie, const time_t& lasttime );
  static void writeAttribute( H5::H5Location& loc,
          const std::string& attr, const std::string& val );
  static void writeAttribute( H5::H5Location& loc,
          const std::string& attr, int val );
  static void writeAttribute( H5::H5Location& loc,
          const std::string& attr, double val );
  static void writeAttributes( H5::H5Location& loc,
          const time_t& start, const time_t& end );
  static void writeAttributes( H5::DataSet& ds, const DataSetDataCache& data,
          double period, double freq );
  static void writeVital( H5::DataSet& ds, H5::DataSpace& space, DataSetDataCache& data );
  static void writeWave( H5::DataSet& ds, H5::DataSpace& space, DataSetDataCache& data, int hz );
  static void autochunk( hsize_t* dims, int rank, hsize_t* rslts );
  static int getHertz( const std::string& wavename );
  static std::unique_ptr<std::vector<int>> resample( const std::string& data, int hz );

  static const std::set<std::string> Hz60;
  static const std::set<std::string> Hz120;
};

#endif /* HDF5WRITER_H */

