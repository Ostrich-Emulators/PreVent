/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CacheFileHdf5Writer.h
 * Author: ryan
 *
 * Created on August 26, 2016, 12:55 PM
 */

#ifndef CPCXMLREADER_H
#define CPCXMLREADER_H

#include "XmlReaderBase.h"
#include <string>
#include <list>
#include <set>

#include "DataRow.h"
namespace FormatConverter {

  class CpcXmlReader : public XmlReaderBase {
  public:

    CpcXmlReader();
    virtual ~CpcXmlReader();

  protected:
    virtual void start(const std::string& element, std::map<std::string, std::string>& attrs) override;
    virtual void end(const std::string& element, const std::string& text) override;
    virtual void comment(const std::string& text) override;

  private:
    const static std::set<std::string> ignorables;
    CpcXmlReader(const CpcXmlReader& orig);
    dr_time currtime;
    dr_time lasttime;
    std::string label;
    std::string value;
    double wavehz;
    int valsperdr;
    double gain;
    bool inmg;
    bool inwave;
    bool inhz;
    bool inpoints;
    bool ingain;
  };
}
#endif /* CPCXMLREADER_H */
