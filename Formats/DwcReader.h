/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WfdbReader.h
 * Author: ryan
 *
 * Created on July 7, 2017, 2:57 PM
 */

#ifndef UMWFDBREADER_H
#define UMWFDBREADER_H

#include "WfdbReader.h"
#include <fstream>
#include "dr_time.h"

namespace FormatConverter {

  /**
   * DWC (Data Warehouse Connect) format uses a WFDB dataset for waveforms
   * and a CSV file for vitals/calculated values
   */
  class DwcReader : public WfdbReader {
  public:
    DwcReader( );

    virtual ~DwcReader( );

  protected:
    int prepare( const std::string& input,SignalSet * info ) override;
    ReadResult fill( SignalSet * data, const ReadResult& lastfill ) override;

  private:
    static const size_t FIRST_VITAL_COL;
    static const size_t DATE_COL;
    static const size_t TIME_COL;
    std::ifstream numerics;
    std::vector<std::string> headings;
    std::map<std::string, std::map<std::string, std::vector<TimedData>>> annomap;
    std::vector<TimedData> clocktimes;

    /**
     * Gets a mapping of vital name to value. The value is in all cases expected
     * to be a number
     * @param csvline
     * @return
     */
    std::map<std::string, std::string> linevalues( const std::string& csvline, dr_time& time );

    static dr_time converttime( const std::string& timeline );
  };
}
#endif /* UMWFDBREADER_H */

