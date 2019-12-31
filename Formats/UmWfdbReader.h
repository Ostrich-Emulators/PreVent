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

  class UmWfdbReader : public WfdbReader {
  public:
    UmWfdbReader( );

    virtual ~UmWfdbReader( );

  protected:
    int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;
    ReadResult fill( std::unique_ptr<SignalSet>& data, const ReadResult& lastfill ) override;

  private:
    static const size_t FIRST_VITAL_COL;
    static const size_t DATE_COL;
    static const size_t TIME_COL;
    std::ifstream numerics;
    std::vector<std::string> headings;

		/**
		 * Splits the CSV line into component strings
		 * @param csvline
		 * @return
		 */
    static std::vector<std::string> splitcsv( const std::string& csvline );

    /**
     * Gets a mapping of vital name to value. The value is in all cases expected
		 * to be a number
     * @param csvline
     * @return
     */
    std::map<std::string, std::string> linevalues( const std::string& csvline, dr_time& time );
  };
}
#endif /* UMWFDBREADER_H */

