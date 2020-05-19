/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   OutputSignalData.h
 * Author: ryan
 *
 * Created on April 10, 2019, 8:37 AM
 */

#ifndef STATISTICALSIGNALDATA_H
#define STATISTICALSIGNALDATA_H

#include "SignalDataWrapper.h"
#include <map>
#include <vector>

namespace FormatConverter {

  /**
   * A SignalData that calculates descriptive statistics as data is added to it
   */
  class StatisticalSignalData : public SignalDataWrapper {
  public:
    StatisticalSignalData(const std::unique_ptr<SignalData>& data);
    StatisticalSignalData(SignalData * data);
    virtual ~StatisticalSignalData( );

    virtual bool add( std::unique_ptr<DataRow> row ) override;

    double mean( ) const;
    double stddev( ) const;
		double variance( ) const;
    double min( ) const;
    double max( ) const;
    double median( ) const;
		size_t count() const;
    /**
     * Gets the mode of this data set. If the dataset is multimodal, only
     * one of the modes is returned
     * @return
     */
    double mode( ) const;
    std::vector<double> modes( ) const;

  private:
    double total;
    size_t _count;
    double _min;
    double _max;
    std::map<double, size_t> numcounts;
  };
}

#endif /* STATISTICALSIGNALDATA_H */
