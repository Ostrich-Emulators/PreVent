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

  int prepare( const std::string& input, SignalSet& info ) override;
  void finish( ) override;
  ReadResult fill( SignalSet& data,
       const ReadResult& lastresult = ReadResult::FIRST_READ ) override;

protected:
  size_t getSize( const std::string& input ) const override;

private:
  Hdf5Reader( const Hdf5Reader& );

  std::string metastr( const H5::Attribute& attr ) const;
  void copymetas( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset ) const;
  void fillVital( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
      const std::vector<dr_time>& times, int valsPerTime, int scale ) const;
  void fillWave( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
      const std::vector<dr_time>& tmes, int valsPerTime, int scale ) const;
  void readDataSet( H5::Group& group, const std::string& name, const bool& iswave,
      SignalSet& info, H5::Group& timegroup ) const;
  std::vector<dr_time> readTimes( H5::Group& timegroup, const std::string& name,
      int& readingsPerTime ) const;
  std::string upgradeMetaKey( const std::string& oldkey ) const;

  H5::H5File file;
};

#endif /* HDF5READER_H */

