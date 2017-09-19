/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Db.h
 * Author: ryan
 *
 * Created on September 18, 2017, 9:18 PM
 */

#ifndef DB_H
#define DB_H

#include <memory>
#include <string>
#include <sqlite3.h>

#include "ConversionListener.h"

class SignalSet;

class Db : public ConversionListener {
public:
	Db( const std::string& fileloc );
	virtual ~Db( );

	virtual void onFileCompleted( const std::string& filename,
			const SignalSet& data ) override;
	virtual void onConversionCompleted( const std::string& input,
			const std::vector<std::string>& outputs ) override;

private:
	sqlite3 * ptr;
};


#endif /* DB_H */

