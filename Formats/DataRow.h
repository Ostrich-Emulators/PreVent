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
  DataRow( const dr_time& time, const std::string& data,
      const std::string& high = "", const std::string& low = "",
      std::map<std::string, std::string> extras = std::map<std::string, std::string>( ) );
  DataRow( );
  DataRow( const DataRow& orig );
  DataRow& operator=(const DataRow& orig );

  void clear( );

  /**
   * convert our "data" value into list of ints. WARNING: if the data isn't an
   * int, we will lose precision. This function is most useful for converting
   * wave datapoints
   * @return
   */
  std::vector<int> ints( ) const;
  static std::vector<int> ints( const std::string& );
  static std::vector<short> shorts( const std::string&, int scale = 1 );


  virtual ~DataRow( );

  /**
   * Figures out how many decimal places are in these numbers.
   * Special case: if the number is 0.0999999 (Philips does this occasionally),
   * the scale of that number is 10 (rounds to 0.1). We do this because we always
   * expect the value to be a short int, and if we multiply all the other numbers
   * by 10000000, we won't get there
   * @param val the value string (will be converted to a float for comparison
   * @param iswave should this value be checked for comma-separated values?
   * @return a power of 10 for small numbers, or a negative power for big numbers
   */
  static int scale( const std::string& val, bool iswave );

  std::string data;
  std::string high;
  std::string low;
  std::map<std::string, std::string> extras;
  dr_time time;
};

#endif /* DATAROW_H */

