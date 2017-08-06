/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignalUtils.h
 * Author: ryan
 *
 * Created on July 28, 2017, 7:41 AM
 */

#ifndef SIGNALUTILS_H
#define SIGNALUTILS_H

#include <map>
#include <string>
#include <memory>
#include <vector>

class SignalData;
class DataRow;

class SignalUtils {
public:
	virtual ~SignalUtils( );

	/**
	 * Aligns all the SignalData caches so they start and end at the same timestamps.
	 * This function consumes the data in its argument
	 * @param map the original SignalData. This gets consumed in this function
	 * @return a map of SignalDatas, where each member has the same start and end 
	 * times
	 */
	static std::vector<std::unique_ptr<SignalData>> sync(
			std::vector<std::unique_ptr<SignalData> >&map );
	/**
	 * Consumes the signal data and creates vector data data points such that the
	 * outside vector is the timestep, and the inside vector is each signal's
	 * data value (as a string, since only the caller knows how to convert the
	 * values).
	 * @param map the map to consume
	 * @return essentially, a 2D vector
	 */
	static std::vector<std::vector<std::string>> syncDatas(
			std::vector<std::unique_ptr<SignalData> >&map );

	static std::vector<std::unique_ptr<SignalData>> vectorize(
			std::map<std::string, std::unique_ptr<SignalData>>&data );

	static std::map<std::string, std::unique_ptr<SignalData>> mapify(
			std::vector<std::unique_ptr<SignalData>>&data );

	/**
	 * Gets the earliest and latest timestamps from the SignalData.
	 * @param map the signals to check
	 * @param earliest the earliest date in the SignalData
	 * @param latest the latest date in the SignalData
	 * @return the earliest date
	 */
	static time_t firstlast( const std::map<std::string, std::unique_ptr<SignalData>>&map,
			time_t * first = nullptr, time_t * last = nullptr );
	static time_t firstlast( const std::vector<std::unique_ptr<SignalData>>&map,
			time_t * first = nullptr, time_t * last = nullptr );

private:
	SignalUtils( );
	SignalUtils( const SignalUtils& );

	static void fillGap( std::unique_ptr<SignalData>& data,
			std::unique_ptr<DataRow>& row, time_t& nexttime, const int& timestep );

};

#endif /* SIGNALUTILS_H */
