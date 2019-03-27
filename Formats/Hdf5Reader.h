/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WfdbReader.h
 * Author: ryan
 *
 * Created on July 7, 2017, 2:57 PM
 */

#ifndef HDF5READER_H
#define HDF5READER_H

#include "Reader.h"

#include <H5Cpp.h>
#include <map>
#include <set>

class Hdf5Reader : public Reader {
public:
  Hdf5Reader( );
  virtual ~Hdf5Reader( );
  static const std::set<std::string> IGNORABLE_PROPS;

  int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
  void finish( ) override;
  ReadResult fill( std::unique_ptr<SignalSet>& data,
      const ReadResult& lastresult = ReadResult::FIRST_READ ) override;

  virtual bool getAttributes( const std::string& inputfile, std::map<std::string, std::string>& map ) override;

  /**
   * Gets a segment of data based on the from and to times.
   * @param inputfile
   * @param path
   * @param from
   * @param to
   * @return
   */
  virtual std::unique_ptr<SignalData> splice( const std::string& inputfile,
      const std::string& path, dr_time from, dr_time to ) override;

private:
  Hdf5Reader( const Hdf5Reader& );

  /**
   * Reads an attribute as a string (converts appropriately)
   * @param attr
   * @return 
   */
  static std::string metastr( const H5::Attribute& attr );
  static std::string metastr( const H5::H5Location& loc, const std::string& attrname );
  static int metaint( const H5::H5Location& loc, const std::string& attrname );

  void copymetas( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset ) const;
  void fillVital( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
      const std::vector<dr_time>& times, int valsPerTime, int timeinterval, int scale ) const;
  void fillWave( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
      const std::vector<dr_time>& tmes, int valsPerTime, int scale ) const;
  void readDataSet( H5::Group& dataAndTimeGroup, const bool& iswave,
      std::unique_ptr<SignalSet>& info ) const;
  std::vector<dr_time> readTimes( H5::DataSet& times ) const;

  /**
   * Gets a single number representing the major/minor/revision nuumbers for
   * the given file. The number is calculated as (major * 10000)+(minor * 100)+revision
   *
   * @param file
   * @return 
   */
  static unsigned int layoutVersion( const H5::H5File& file );

  /**
   * Find the index for the given time in the given dataset. If the time does 
   * not exist in the dataset, return the index where it *would* be if it existed
   * @param haystack
   * @param needle
   * @return 
   */
  static hsize_t getIndexForTime( H5::DataSet& haystack, dr_time needle, bool * found = nullptr );

  /**
   * Reads (as ints) the given dataset from start (inclusive) to end (exclusive)
   * @param data
   * @param startidx the first row of data to retrieve
   * @param endidx the index after the last row to retrieve
   * @return 
   */
  static std::vector<int> slabreadi( H5::DataSet& data, hsize_t startidx, hsize_t endidx );
  /**
   * Reads (as shorts) the given dataset from start (inclusive) to end (exclusive)
   * @param data
   * @param startidx the first row of data to retrieve
   * @param endidx the index after the last row to retrieve
   * @return
   */
  static std::vector<int> slabreads( H5::DataSet& data, hsize_t startidx, hsize_t endidx );
    /**
   * Reads (as longs) the given dataset from start (inclusive) to end (exclusive)
   * @param data
   * @param startidx the first row of data to retrieve
   * @param endidx the index after the last row to retrieve
   * @return
   */
  static std::vector<dr_time> slabreadt( H5::DataSet& data, hsize_t startidx, hsize_t endidx );

  H5::H5File file;
};

#endif /* HDF5READER_H */

