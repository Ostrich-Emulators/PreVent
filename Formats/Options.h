/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Options.h
 * Author: ryan
 *
 * Created on February 12, 2019, 7:26 PM
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <map>

enum OptionsKey {
	NOCACHE
};

class Options {
public:
	virtual ~Options( );


	static void set( OptionsKey key, const std::string& val );
	static void set( OptionsKey key, bool val );
	static std::string get( OptionsKey key );
	static bool asBool( OptionsKey key );

private:
	Options( );

	static std::map<OptionsKey, std::string> map;

};

#endif /* OPTIONS_H */

