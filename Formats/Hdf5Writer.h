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


class SignalData;

class Hdf5Writer : public Writer {
public:
  static const std::string LAYOUT_VERSION;
  Hdf5Writer( );
  virtual ~Hdf5Writer( );

protected:
  std::vector<std::string> closeDataSet( );
  int drain( SignalSet& );

private:

  Hdf5Writer( const Hdf5Writer& orig );

  static void writeFileAttributes( H5::H5File file, std::map<std::string, std::string> datasetattrs,
        const dr_time& firsttime, const dr_time& lasttime );
  static void writeAttribute( H5::H5Location& loc,
        const std::string& attr, const std::string& val );
  static void writeAttribute( H5::H5Location& loc,
        const std::string& attr, int val );
  static void writeAttribute( H5::H5Location& loc,
        const std::string& attr, double val );
  static void writeAttribute( H5::H5Location& loc,
        const std::string& attr, dr_time val );
  static void writeTimesAndDurationAttributes( H5::H5Location& loc,
        const dr_time& start, const dr_time& end );
  static void writeAttributes( H5::H5Location& ds, const SignalData& data );
  void writeVital( H5::DataSet& ds, H5::DataSpace& space, SignalData& data );
  void writeVitalGroup( H5::Group& group, SignalData& data );
  void writeWave( H5::DataSet& ds, H5::DataSpace& space, SignalData& data );
  void writeWaveGroup( H5::Group& group, SignalData& data );
  void writeTimes( H5::Group& group, SignalData& data );
  void writeGroupAttrs( H5::Group& group, SignalData& data );
  static void autochunk( hsize_t* dims, int rank, hsize_t* rslts );
  void createEventsAndTimes( H5::H5File, const SignalSet& data );

  /**
   * Rescale the data to fit in shorts
   * @param data
   * @return true, if the data was rescaled
   */
  bool rescaleIfNeeded( SignalData& data ) const;
  SignalSet * dataptr;
};

#endif /* HDF5WRITER_H */

