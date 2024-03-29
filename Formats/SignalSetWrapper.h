/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SignalSetWrapper.h
 * Author: ryan
 *
 * Created on July 2, 2018, 3:08 PM
 */

#ifndef SIGNALSETWRAPPER_H
#define SIGNALSETWRAPPER_H

#include "SignalSet.h"

#include <memory>
namespace FormatConverter {

  class SignalSetWrapper : public SignalSet {
  public:
    SignalSetWrapper( const std::unique_ptr<SignalSet>& ss );
    SignalSetWrapper( SignalSet * ss );
    virtual ~SignalSetWrapper( );

    /**
     * Adds a new vital sign if it has not already been added. If it already
     * exists, the old dataset is returned
     * @param name
     * @param if not NULL, will be set to true if this is the first time this function
     *  has been called for this vital
     * @return
     */
    virtual SignalData * addVital( const std::string& name, bool * added = nullptr ) override;
    /**
     * Adds a new waveform if it has not already been added. If it already
     * exists, the old dataset is returned
     * @param name
     * @param if not NULL, will be set to true if this is the first time this function
     *  has been called for this vital
     * @return
     */
    virtual SignalData * addWave( const std::string& name, bool * added = nullptr ) override;

    virtual std::vector<SignalData *> vitals( ) const override;
    virtual std::vector<SignalData *> waves( ) const override;

    virtual void reset( bool signalDataOnly = true ) override;
    virtual void setMeta( const std::string& key, const std::string& val ) override;
    virtual dr_time earliest( const TimeCounter& tc = EITHER ) const override;
    virtual dr_time latest( const TimeCounter& tc = EITHER ) const override;
    virtual const std::map<std::string, std::string>& metadata( ) const override;


    virtual const std::map<long, dr_time>& offsets( ) const override;
    virtual void addOffset( long seg, dr_time time ) override;
    virtual void clearOffsets( ) override;

    virtual std::vector<SignalData *> allsignals( ) const override;
    virtual void clearMetas( ) override;

    virtual void setMetadataFrom( const SignalSet& target ) override;

    virtual void complete( ) override;

    virtual void addAuxillaryData( const std::string& name, const TimedData& data ) override;
    virtual std::map<std::string, std::vector<TimedData>> auxdata( ) override;

    virtual std::unique_ptr<SignalData> _createSignalData( const std::string& name,
        bool iswave = false, void * extra = nullptr ) override;

    virtual bool empty() const;

  private:
    SignalSet * set;
    bool iOwnThisPointer;
  };
}
#endif /* SIGNALSETWRAPPER_H */

