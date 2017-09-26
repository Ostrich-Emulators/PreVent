/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ConversionListener.h
 * Author: ryan
 *
 * Created on September 19, 2017, 4:23 PM
 */

#ifndef CONVERSIONLISTENER_H
#define CONVERSIONLISTENER_H

class SignalSet;

#include <string>
#include <vector>

enum ConversionProperty{ QUIET };

class ConversionListener {
public:

	virtual ~ConversionListener( ) {
	}

	virtual void onFileCompleted( const std::string& filename, const SignalSet& data ) = 0;
	virtual void onConversionCompleted( const std::string& input,
			const std::vector<std::string>& outputs ) = 0;
	virtual void setProperty( ConversionProperty key, const std::string& val ) = 0;
};



#endif /* CONVERSIONLISTENER_H */

