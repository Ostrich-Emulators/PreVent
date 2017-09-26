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
#include <map>
#include <sqlite3.h>

#include "ConversionListener.h"

class SignalSet;
class SignalData;

class Db : public ConversionListener {
public:
	Db( );
	virtual ~Db( );

	void init( const std::string& fileloc );

	virtual void onFileCompleted( const std::string& filename,
			const SignalSet& data ) override;
	virtual void onConversionCompleted( const std::string& input,
			const std::vector<std::string>& outputs ) override;

private:
	sqlite3 * ptr;
	static const std::string CREATE;
	// unit-bed -> id
	std::map<std::pair<std::string, std::string>, int> bedids;
	std::map<std::string, int> unitids;
	std::map<std::string, int> signalids;
	std::map<std::string, int> patientids;

	static int nameidcb( void *a_param, int argc, char **argv, char **column );
	static int bedcb( void *a_param, int argc, char **argv, char **column );

	void exec( const std::string& sql, void * callback = nullptr, void * param = nullptr );
	int addPatient( const std::string& name );
	void addSignal( int fileid, const SignalData& sig );
};


#endif /* DB_H */

