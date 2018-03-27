/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DurationSignalData.h
 * Author: ryan
 *
 * Created on March 26, 2018, 4:39 PM
 */

#ifndef DURATIONSIGNALDATA_H
#define DURATIONSIGNALDATA_H

#include "SignalData.h"

class DurationSpecification;

/**
 * A wrapper class around some other SignalData that specifies a valid
 * duration. DateRows that fall outside the valid specification are silently ignored
 * @param data
 * @param spec
 */
class DurationSignalData : public SignalData {
public:
  DurationSignalData( std::unique_ptr<SignalData> data, const DurationSpecification& spec);
  virtual ~DurationSignalData();

  virtual std::unique_ptr<SignalData> shallowcopy(bool includedates = false) override;
  virtual void moveDataTo(std::unique_ptr<SignalData>& signal) override;
  virtual void add(const DataRow& row) override;
  virtual void setUom(const std::string& u) override;
  virtual const std::string& uom() const override;
  virtual int scale() const override;
  virtual size_t size() const override;
  virtual double hz() const override;
  virtual const dr_time& startTime() const override;
  virtual const dr_time& endTime() const override;
  virtual const std::string& name() const override;
  virtual void setValuesPerDataRow(int) override;
  virtual int valuesPerDataRow() const override;
  virtual void setMetadataFrom(const SignalData& model) override;

  virtual std::unique_ptr<DataRow> pop() override;
  virtual bool empty() const override;
  virtual void setWave(bool wave = false) override;
  virtual bool wave() const override;

  virtual std::map<std::string, std::string>& metas() override;
  virtual std::map<std::string, int>& metai() override;
  virtual std::map<std::string, double>& metad() override;
  virtual const std::map<std::string, std::string>& metas() const override;
  virtual const std::map<std::string, int>& metai() const override;
  virtual const std::map<std::string, double>& metad() const override;
  virtual const std::deque<dr_time>& times() const override;
  virtual std::vector<std::string> extras() const override;
  
private:
  std::unique_ptr<SignalData> signal;
  const DurationSpecification& spec;
};

#endif /* DURATIONSIGNALDATA_H */

