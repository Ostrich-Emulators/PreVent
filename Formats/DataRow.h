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
    static DataRow from( const dr_time&, const std::string& data );
    static DataRow one( const dr_time&, const std::string& data );
    static DataRow many( const dr_time&, const std::string& data );

    DataRow( const dr_time& time, const std::vector<int>& data, int scale = 0,
        std::map<std::string, std::string> extras = std::map<std::string, std::string>( ) );
    DataRow( const dr_time& time, int data, int scale = 0,
        std::map<std::string, std::string> extras = std::map<std::string, std::string>( ) );
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

    static int scaleOf( const std::string& data );

    dr_time time;
    std::vector<int> data;
    int scale;
    std::map<std::string, std::string> extras;

  private:
    static const std::set<std::string> hiloskips;
    static void intify( const std::string_view& data, size_t dotpos, int * val, int * scale );
  };
}
#endif /* DATAROW_H */

