/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SignalSet.h"
#include "BasicSignalData.h"
#include "SignalUtils.h"

#include <limits>
#include <iostream>

namespace FormatConverter{

  SignalSet::SignalSet( ) { }

  SignalSet::~SignalSet( ) { }

  SignalSet::AuxData::AuxData( dr_time t, const std::string& v ) : ms( t ), val( v ) { }

  SignalSet::AuxData::~AuxData( ) { }
}
