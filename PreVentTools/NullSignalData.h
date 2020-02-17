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

#ifndef NULLSIGNALDATA_H
#define NULLSIGNALDATA_H

#include "BasicSignalData.h"
#include <iostream>
#include <string>

namespace FormatConverter {

	/**
	 * A SignalData that ignores all calls to add() (useful for wrappers that
	 * do calculations on data, but don't need the data once the calculation is complete)
	 */
	class NullSignalData : public BasicSignalData {
	public:
		NullSignalData( const std::string& name = "-", bool iswave = false );
		virtual ~NullSignalData( );
		virtual void add( const FormatConverter::DataRow& row ) override;
	};
}

#endif /* NULLSIGNALDATA_H */
