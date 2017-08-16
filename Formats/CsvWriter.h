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


class SignalData;

class CsvWriter : public Writer {
public:
  CsvWriter();
  virtual ~CsvWriter();

  static const int MISSING_VALUE;

protected:
  int initDataSet(const std::string& outdir, const std::string& namestart,
          int compression);
  std::vector<std::string> closeDataSet();
  int drain(SignalSet&);

private:
  CsvWriter(const CsvWriter& orig);
private:
  std::string filestart;
  std::string filename;
};

#endif /* CSVWRITER_H */

