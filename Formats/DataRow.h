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

#include "dr_time.h"

class DataRow {
public:
  DataRow( );
  DataRow( const dr_time& time, const std::string& data,
        std::map<std::string, std::string> extras = std::map<std::string, std::string>( ) );
  DataRow( const DataRow& orig );
  DataRow& operator=(const DataRow& orig );

  void clear( );

  /**
   * Converts the given data string (comma-separated or not) to integers by
   * multiplying the each value by the scale.
   * @return
   */
  static std::vector<int> ints( const std::string&, int scale = 1 );
  static std::vector<short> shorts( const std::string&, int scale = 1 );

  std::vector<int> ints( int scale = 1 ) const;
  std::vector<short> shorts( int scale = 1 ) const;

  /**
   * Iterates through the given string and determines the highest and lowest vals
   * higher/lower than the given values. That is, if a highval is 10, the vals
   * contains 11, the highval is set to 11. If vals only contained 9, then highval
   * would remain 10.
   * @param highval
   * @param lowval
   */
  static void hilo( const std::string& vals, double& highval, double& lowval );

  virtual ~DataRow( );

  /**
   * Figures out the appropriate power of 10 to multiply a value (or csv) to
   * convert it from a double to an integer. 
   * 
   * @param val the value string (will be converted to a float for comparison)
   * @param iswave should this value be checked for comma-separated values?
   * @return a power of 10 for small numbers, or a negative power for big numbers
   */
  static int scale( const std::string& val, bool iswave );

  std::string data;
  std::map<std::string, std::string> extras;
  dr_time time;
};

#endif /* DATAROW_H */

