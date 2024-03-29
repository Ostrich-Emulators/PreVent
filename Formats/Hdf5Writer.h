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
#include "DataRow.h"

namespace FormatConverter {
  class SignalSet;
  class SignalData;
  class TimeRange;

  class Hdf5Writer : public Writer {
  public:
    static const std::string LAYOUT_VERSION;
    Hdf5Writer( );
    virtual ~Hdf5Writer( );

    static void writeAttribute( H5::H5Object& loc, const std::string& attr, const std::string& val );
    static void writeAttribute( H5::H5Object& loc, const std::string& attr, int val );
    static void writeAttribute( H5::H5Object& loc, const std::string& attr, double val );
    static void writeAttribute( H5::H5Object& loc, const std::string& attr, dr_time val );
    static std::string getDatasetName( SignalData * data );
    static std::string getDatasetName( const std::string& oldname );
    static void autochunk( hsize_t* dims, int rank, int bytesperdim, hsize_t* rslts );
    static void writeAttributes( H5::H5Object& ds, SignalData * data );

    /**
     * Drains a single dataset into the given group. This is useful for
     * appending data to an existing file
     * @param g
     * @param
     * @return
     */
    void drain( H5::Group& g, SignalData * );

    static H5::Group ensureGroupExists( H5::H5Object& obj, const std::string& s );

  protected:
    std::vector<std::string> closeDataSet( );
    int drain( SignalSet * );

  private:
    Hdf5Writer( const Hdf5Writer& orig );

    void writeFileAttributes( H5::H5File file, std::map<std::string, std::string> datasetattrs,
        const dr_time& firsttime, const dr_time& lasttime );
    void writeTimesAndDurationAttributes( H5::H5Object& loc,
        const dr_time& start, const dr_time& end );
    void writeVital( H5::Group& group, SignalData * data );
    void writeVitalGroup( H5::Group& group, SignalData * data );
    void writeWave( H5::Group& group, SignalData * data );
    void writeWaveGroup( H5::Group& group, SignalData * data );
    void writeTimes( H5::Group& group, SignalData * data );
    H5::DataSet writeTimes( H5::Group& group, TimeRange * data, const std::string& dsname = "time" );
    void writeEvents( H5::Group& group, SignalData * data );
    void writeAuxData( H5::Group& group, const std::string& name, const std::vector<TimedData>& data );
    void writeGroupAttrs( H5::Group& group, SignalData * data );
    void createEventsAndTimes( H5::H5File, const SignalSet * data );

    /**
     * Can the data be fit into a short int for storage?
     * @param data
     * @return true if the caller should use shorts instead of integers
     * for saving data to dataset
     */
    bool shortsOk( SignalData * data ) const;
    void createTimeCache( );
    SignalSet * dataptr;
    std::map<dr_time, long> timesteplkp;
    std::map<SignalData *, std::unique_ptr<TimeRange>> timecache;
  };
}
#endif /* HDF5WRITER_H */

