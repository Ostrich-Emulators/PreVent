/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataRow.h
 * Author: ryan
 *
 * Created on August 3, 2016, 7:50 AM
 */

#ifndef DATAROW_H
#define DATAROW_H

#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

#include "dr_time.h"
namespace FormatConverter {

  class TimedData {
  public:
    dr_time time;
    std::string data;

    TimedData( dr_time m, const std::string& v );
    TimedData( const TimedData& orig );
    TimedData& operator=(const TimedData& orig );
    virtual ~TimedData( );
  };

  class DataRow {
  public:
    /**
     * Creates a new DataRow from the given string. The string is decoded into
     * either a single data value, or a CSV string of values. This function is
     * a convenience function to one(), and many(), depending on whether or not
     * the data string contains a ,
     * @param
     * @param data
     * @return
     */
    static std::unique_ptr<DataRow> from( const dr_time&, const std::string& data );
    static std::unique_ptr<DataRow> one( const dr_time&, const std::string& data );
    static std::unique_ptr<DataRow> many( const dr_time&, const std::string& data );

    DataRow( const dr_time& time, const std::vector<int>& data, int scale = 0,
        std::map<std::string, std::string> extras = std::map<std::string, std::string>( ) );
    DataRow( const dr_time& time, int data, int scale = 0,
        std::map<std::string, std::string> extras = std::map<std::string, std::string>( ) );
    DataRow( const dr_time& time, const std::vector<double>& data,
        std::map<std::string, std::string> extras = std::map<std::string, std::string>( ),
        int maxprecision = 3 );

    DataRow( const DataRow& orig );
    DataRow& operator=(const DataRow& orig );
    virtual ~DataRow( );

    void clear( );

    const std::vector<int>& ints( ) const;
    std::vector<short> shorts( ) const;
    std::vector<double> doubles( ) const;

    /**
     * Copies this DataRow at a different scale
     * @param newscale
     * @return
     */
    DataRow newscale( int newscale ) const;

    /**
     * Updates values in this DataRow to the new scale (up or down)
     * Note that rescaling down may lead to loss of precision
     * @param newscale
     */
    void rescale( int newscale );

    /**
     * Finds the scale of a single data value (not a CSV)
     * @param data
     * @return
     */
    static int scaleOf( const std::string& data );

    dr_time time;
    std::vector<int> data;
    int scale;
    std::map<std::string, std::string> extras;

  private:
    static const std::set<std::string> hiloskips;

    /**
     * Converts a string to a single integer value 
     * @param data
     * @param dotpos
     * @param val
     * @param scale
     */
    static void intify( const std::string_view& data, size_t dotpos, int * val, int * scale );

    /**
     * Converts a string into multiple integers, and loads them into the given vector
     * @param data
     * @param vec
     * @param scale
     */
    static void intify( const std::string_view& data, std::vector<int>& vec, int * scale );
  };
}
#endif /* DATAROW_H */

