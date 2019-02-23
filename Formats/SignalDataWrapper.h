/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   A base class for wrapping other SignalData instances
 * Author: ryan
 *
 * Created on March 26, 2018, 4:39 PM
 */

#ifndef SIGNALDATAWRAPPER_H
#define SIGNALDATAWRAPPER_H

#include "SignalData.h"

/**
 * A base class for other wrappers. Every function call gets passed to the base
 * SignalData
 */
class SignalDataWrapper : public SignalData {
public:
  /**
   * Creates a new wrapper around this signaldata, but does not take ownership
   * @param data
   */
  SignalDataWrapper( const std::unique_ptr<SignalData>& data );

  /**
   * Creates a new Wrapper around this pointer AND TAKES OWNERSHIP OF IT
   * @param data
   */
  SignalDataWrapper( SignalData * data );
  virtual ~SignalDataWrapper( );

  virtual std::unique_ptr<SignalData> shallowcopy( bool includedates = false ) override;
  virtual void moveDataTo( std::unique_ptr<SignalData>& signal ) override;
  virtual void add( const DataRow& row ) override;
  virtual void setUom( const std::string& u ) override;
  virtual const std::string& uom( ) const override;
  virtual int scale( ) const override;
  virtual size_t size( ) const override;
  virtual double hz( ) const override;
  virtual dr_time startTime( ) const override;
  virtual dr_time endTime( ) const override;
  virtual const std::string& name( ) const override;
  virtual int readingsPerSample( ) const override;
  virtual void setChunkIntervalAndSampleRate( int chunk_ms, int sr ) override;
  virtual void setMetadataFrom( const SignalData& model ) override;

  virtual double highwater( ) const override;
  virtual double lowwater( ) const override;

  virtual std::unique_ptr<DataRow> pop( ) override;
  virtual bool empty( ) const override;
  virtual void setWave( bool wave = false ) override;
  virtual bool wave( ) const override;

  virtual void setMeta( const std::string& key, const std::string& val ) override;
  virtual void setMeta( const std::string& key, int val ) override;
  virtual void setMeta( const std::string& key, double val ) override;
  virtual void erases( const std::string& key = "" ) override;
  virtual void erasei( const std::string& key = "" ) override;
  virtual void erased( const std::string& key = "" ) override;


  virtual const std::map<std::string, std::string>& metas( ) const override;
  virtual const std::map<std::string, int>& metai( ) const override;
  virtual const std::map<std::string, double>& metad( ) const override;
  virtual const std::deque<dr_time> times( ) const override;
  virtual std::vector<std::string> extras( ) const override;
  virtual void extras( const std::string& ext ) override;

  virtual void recordEvent( const std::string& eventtype, const dr_time& time ) override;
  virtual std::vector<std::string> eventtypes( ) override;
  virtual std::vector<dr_time> events( const std::string& eventtype ) override;

  virtual size_t inmemsize( ) const override;
private:
  SignalData * signal;
  bool iOwnThisPtr;
};

#endif /* SIGNALDATAWRAPPER_H */

