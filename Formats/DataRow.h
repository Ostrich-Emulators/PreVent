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

class DataRow {
public:
	DataRow( const time_t& time, const std::string& data,
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
	std::vector<short> shorts( ) const;
	static std::vector<int> ints( const std::string& );
	static std::vector<short> shorts( const std::string& );


	virtual ~DataRow( );

	static int scale( const std::string& val );

	std::string data;
	std::string high;
	std::string low;
	std::map<std::string, std::string> extras;
	time_t time;
};

#endif /* DATAROW_H */

