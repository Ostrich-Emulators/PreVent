/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * A class to "listen" for events while reading, similar to SAX XML parsing
 * Note that if listeners are enabled, then the parser does not store the
 * values for later reference. We're trying to save memory here.
 *
 * File:   TdmsListener.h
 * Author: ryan
 *
 * Created on February 17, 2019, 9:13 PM
 */

#ifndef TDMSLISTENER_H
#define TDMSLISTENER_H

#include <string>
#include <vector>

class TdmsChannel;

class TdmsListener {
public:
	virtual void newGroup( TdmsGroup * grp ) = 0;
	virtual void newChannel( TdmsChannel * channel ) = 0;
	/**
	 * notify listeners of new value.
	 * @param channel
	 * @param val
	 * @return true, if the reader should push this value to its internal data vector
	 */
	virtual void newValueChunk( TdmsChannel * channel, std::vector<double>& val ) = 0;

	virtual void newValueChunk( TdmsChannel * channel, std::vector<std::string>& val ) {
	};

	virtual void newImaginaryValueChunk( TdmsChannel * channel, std::vector<double>& val ) {
	};
};

#endif /* TDMSLISTENER_H */

