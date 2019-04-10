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

#ifndef OUTPUTSIGNALDATA_H
#define OUTPUTSIGNALDATA_H

#include "BasicSignalData.h"
#include <iostream>
#include <string>

/**
 * A SignalData that just prints to the ostream whatever data is added to it t
 */
class OutputSignalData : public BasicSignalData {
public:
	OutputSignalData( std::ostream& out );
	virtual ~OutputSignalData( );

	virtual void add( const DataRow& row ) override;

private:
	std::ostream& output;
};

#endif /* OUTPUTSIGNALDATA_H */

