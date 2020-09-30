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
#include "TimeRange.h"

#include <H5Cpp.h>
#include <map>
#include <set>
namespace FormatConverter {

  class Hdf5Reader : public Reader {
  public:
    Hdf5Reader( );
    virtual ~Hdf5Reader( );
    static const std::set<std::string> IGNORABLE_PROPS;

    int prepare( const std::string& input, SignalSet * info ) override;
    void finish( ) override;
    ReadResult fill( SignalSet * data, const ReadResult& lastresult = ReadResult::FIRST_READ ) override;

    virtual bool getAttributes( const std::string& inputfile, std::map<std::string, std::string>& map ) override;

    virtual bool getAttributes( const std::string& inputfile, const std::string& signal,
        std::map<std::string, int>& mapi, std::map<std::string, double>& mapd, std::map<std::string, std::string>& maps,
        dr_time& start, dr_time& end ) override;

    /**
     * Gets a segment of data based on the from and to times.
     * @param inputfile
     * @param path
     * @param from
     * @param to
     * @return
     */
    virtual bool splice( const std::string& inputfile, const std::string& path,
        dr_time from, dr_time to, SignalData * signal ) override;

  private:
    Hdf5Reader( const Hdf5Reader& );

    class SignalSaver {
    public:
      std::vector<dr_time> times;
      hsize_t lastrowread;
      bool nomorerows;

      SignalSaver( std::vector<dr_time> timesleft = std::vector<dr_time>{ }, hsize_t lastrowread = 0 );
    };


    /**
     * Reads an attribute as a string (converts appropriately)
     * @param attr
     * @return
     */
    static std::string metastr( const H5::Attribute& attr );
    static std::string metastr( const H5::H5Object& loc, const std::string& attrname );
    static int metaint( const H5::H5Object& loc, const std::string& attrname );
    static int metaint( const H5::Attribute& attr );

    static void copymetas( SignalData * signal, H5::H5Object& dataset,
        bool includeIgnorables = false );
    void fillVital( SignalData * signal, SignalSet * info, H5::DataSet& dataset,
        SignalSaver& saver, int valsPerTime, int timeinterval, int scale ) const;
    void fillWave( SignalData * signal, SignalSet * info, H5::DataSet& dataset,
        SignalSaver& saver, int valsPerTime, int scale ) const;
    std::map<std::string, std::vector<TimedData>> readAuxData( H5::Group& auxparent );
    void readDataSet( H5::Group& dataAndTimeGroup, const bool& iswave,
        SignalSet * info );
    std::vector<dr_time> readTimes( H5::DataSet& times );

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
     * @param leftmost if true, return the time leftmost time
     * @return
     */
    static hsize_t getIndexForTime( H5::DataSet& haystack, dr_time needle, bool leftmost );
    static dr_time getTimeAtIndex( H5::DataSet& haystack, hsize_t index );

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
    static std::unique_ptr<TimeRange> slabreadt( H5::DataSet& data, hsize_t startidx, hsize_t endidx );
    /**
     * Reads a small (<512K) number of dates into a vector. If the number is >512K,
     * throws an exception
     * @param data
     * @param startidx
     * @param endidx
     * @return
     */
    static std::vector<dr_time> slabreadt_small( H5::DataSet& data, hsize_t startidx, hsize_t endidx );

    H5::H5File file;
    std::map<std::string, SignalSaver> savers;
  };
}
#endif /* HDF5READER_H */

