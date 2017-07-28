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

class DataRow {
public:
  DataRow( const time_t& time, const std::string& data, 
			const std::string& high = "", const std::string& low = "" );
  DataRow();
  DataRow( const DataRow& orig );
  DataRow& operator=(const DataRow& orig );
  
  void clear();
  
  virtual ~DataRow( );
  
  static int scale( const std::string& val );

  std::string data;
  std::string high;
  std::string low;
  time_t time;
};

#endif /* DATAROW_H */

