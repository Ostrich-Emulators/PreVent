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

#include "dr_time.h"

class SignalSet;
class SignalData;
class DataRow;

class SignalUtils {
public:
  virtual ~SignalUtils( );

  /**
   * Aligns all the SignalData caches so they start and end at the same timestamps.
   * This function consumes the data in its argument
   * @param map the original SignalData. This gets consumed in this function
   * @return a vector of SignalDatas, where each member has the same start and 
   * end times
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
   * Gets all times in the given SignalSet. This is the superset of all 
   * SignalData's timeseries.
   * @param ss
   * @return 
   */
  static std::vector<dr_time> alltimes( const SignalSet& ss );

  /**
   * Creates a vector with index positions for the given signal's data.
   * @param alltimes
   * @param signal
   * @return a vector the same size as alltimes
   */
  static std::vector<size_t> index( const std::vector<dr_time>& alltimes,
      const SignalData& signal );

  /**
   * Gets the earliest and latest timestamps from the SignalData.
   * @param map the signals to check
   * @param earliest the earliest date in the SignalData
   * @param latest the latest date in the SignalData
   * @return the earliest date
   */
  static dr_time firstlast( const std::map<std::string, std::unique_ptr<SignalData>>&map,
      dr_time * first = nullptr, dr_time * last = nullptr );
  static dr_time firstlast( const std::vector<std::unique_ptr<SignalData>>&map,
      dr_time * first = nullptr, dr_time * last = nullptr );


  /**
   * Converts the given val to a string, and removes any trailing 0s (to make the
   * shortest string).
   * @param val
   * @param scalefactor scaling multiplier
   * @return
   */
  static std::string tosmallstring( double val, double scalefactor = 1.0 );

private:
  SignalUtils( );
  SignalUtils( const SignalUtils& );

  static void fillGap( std::unique_ptr<SignalData>& data,
      std::unique_ptr<DataRow>& row, dr_time& nexttime, const int& timestep );
  /**
   * Creates a valid "dummy" DataRow for the given signal. (Wave forms need
   * a series of missing values, while vitals only need one.)
   * @param signal
   * @param time
   * @return
   */
  static DataRow dummyfill( std::unique_ptr<SignalData>& signal, const dr_time& time );
};

#endif /* SIGNALUTILS_H */

