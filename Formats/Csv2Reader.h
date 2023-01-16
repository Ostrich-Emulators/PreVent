/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CsvReader.h
 * Author: ryan
 *
 * Created on September 1, 2020, 9:23 PM
 */

#ifndef CSVREADER2_H
#define CSVREADER2_H

#include "CsvReader.h"

#include <vector>
#include <string>
#include <fstream>
#include "dr_time.h"

namespace FormatConverter {

  class Csv2Reader : public CsvReader {
  public:
    Csv2Reader( );

    virtual ~Csv2Reader( );

  protected:
    int prepare( const std::string& input, SignalSet * info ) override;
    dr_time converttime( const std::string& timeline ) override;
    std::string headerForField( int field, const std::vector<std::string>& linevals ) const override;
    bool includeFieldValue( int field, const std::vector<std::string>& linevals ) const override;
    bool isNewPatient( const std::vector<std::string>& linevals, SignalSet * info ) const override;
    void setMetas( const std::vector<std::string>& linevals, SignalSet * data ) override;
    virtual bool isFirstLineHeader(const std::string& firstline) override;
  private:
    std::map<std::string,std::string> headerlkp;
    int tzmod;
  };
}

#endif /* CSVREADER2_H */

