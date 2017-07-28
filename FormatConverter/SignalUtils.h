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

class SignalData;

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
	static std::map<std::string, std::unique_ptr<SignalData>> sync(
			std::map<std::string, std::unique_ptr<SignalData> >&map );

	/**
	 * Gets the earliest and latest timestamps from the SignalData.
	 * @param map the data to check
	 * @param earliest the earliest date in the SignalData
	 * @param latest the latest date in the SignalData
	 * @return the earliest date
	 */
	static time_t firstlast( const std::map<std::string, std::unique_ptr<SignalData>>&map,
			time_t * first = nullptr, time_t * last = nullptr );

private:
	SignalUtils( );
	SignalUtils( const SignalUtils& );

};

#endif /* SIGNALUTILS_H */

