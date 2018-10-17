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
protected:
  size_t getSize( const std::string& input ) const override;

private:
  Hdf5Reader( const Hdf5Reader& );

  /**
   * Reads an attribute as a string (converts appropriately)
   * @param attr
   * @return 
   */
  std::string metastr( const H5::Attribute& attr ) const;
  std::string metastr( const H5::H5Location& loc, const std::string& attrname ) const;
  int metaint( const H5::H5Location& loc, const std::string& attrname ) const;
  void copymetas( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset ) const;
  void fillVital( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
      const std::vector<dr_time>& times, int valsPerTime, int timeinterval, int scale ) const;
  void fillWave( std::unique_ptr<SignalData>& signal, H5::DataSet& dataset,
      const std::vector<dr_time>& tmes, int valsPerTime, int scale ) const;
  void readDataSet( H5::Group& dataAndTimeGroup, const bool& iswave,
      std::unique_ptr<SignalSet>& info ) const;
  std::vector<dr_time> readTimes( H5::DataSet& times ) const;

  H5::H5File file;
};

#endif /* HDF5READER_H */

