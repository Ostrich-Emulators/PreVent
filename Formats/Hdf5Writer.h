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
#include "Writer.h"
#include "dr_time.h"

class SignalSet;
class SignalData;

class Hdf5Writer : public Writer {
public:
  static const std::string LAYOUT_VERSION;
  Hdf5Writer( );
  virtual ~Hdf5Writer( );

  static void writeAttribute( H5::H5Location& loc,
      const std::string& attr, const std::string& val );
  static void writeAttribute( H5::H5Location& loc,
      const std::string& attr, int val );
  static void writeAttribute( H5::H5Location& loc,
      const std::string& attr, double val );
  static void writeAttribute( H5::H5Location& loc,
      const std::string& attr, dr_time val );
protected:
  std::vector<std::string> closeDataSet( );
  int drain( std::unique_ptr<SignalSet>& );

private:
  Hdf5Writer( const Hdf5Writer& orig );

  void writeFileAttributes( H5::H5File file, std::map<std::string, std::string> datasetattrs,
      const dr_time& firsttime, const dr_time& lasttime );
  void writeTimesAndDurationAttributes( H5::H5Location& loc,
      const dr_time& start, const dr_time& end );
  static void writeAttributes( H5::H5Location& ds, const std::unique_ptr<SignalData>& data );
  void writeVital( H5::Group& group, std::unique_ptr<SignalData>& data );
  void writeVitalGroup( H5::Group& group, std::unique_ptr<SignalData>& data );
  void writeWave( H5::Group& group, std::unique_ptr<SignalData>& data );
  void writeWaveGroup( H5::Group& group, std::unique_ptr<SignalData>& data );
  void writeTimes( H5::Group& group, std::unique_ptr<SignalData>& data );
  void writeEvents( H5::Group& group, std::unique_ptr<SignalData>& data );
  void writeGroupAttrs( H5::Group& group, std::unique_ptr<SignalData>& data );
  static void autochunk( hsize_t* dims, int rank, int bytesperdim, hsize_t* rslts );
  void createEventsAndTimes( H5::H5File, const std::unique_ptr<SignalSet>& data );
  static std::string getDatasetName( const std::unique_ptr<SignalData>& data );
  static std::string getDatasetName( const std::string& oldname );

  /**
   * Rescale the data to fit in shorts
   * @param data
   * @param useIntsNotShorts (output arg) if true, use integers instead of shorts
   * for saving data to dataset
   * @return true, if the data was rescaled
   */
  bool rescaleForShortsIfNeeded( std::unique_ptr<SignalData>& data, bool& useIntsNotShorts ) const;
  void createTimesteps( );
  SignalSet * dataptr;
  std::map<dr_time, long> timesteplkp;
};

#endif /* HDF5WRITER_H */

