/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CsvWriter.h
 * Author: ryan
 *
 * Created on August 26, 2016, 12:58 PM
 */

#ifndef CSVWRITER_H
#define CSVWRITER_H

#include <map>
#include <set>
#include <vector>
#include <string>
#include <memory>
#include <ctime>
#include "Writer.h"

namespace FormatConverter {

  class CsvWriter : public Writer {
  public:
    CsvWriter();
    virtual ~CsvWriter();

    static const int MISSING_VALUE;

  protected:
    std::vector<std::string> closeDataSet();
    int drain(std::unique_ptr<SignalSet>&);

  private:
    CsvWriter(const CsvWriter& orig);
  };
}
#endif /* CSVWRITER_H */

