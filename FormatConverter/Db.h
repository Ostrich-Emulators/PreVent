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
			const std::unique_ptr<SignalSet>& data ) override;
	virtual void onConversionCompleted( const std::string& input,
			const std::vector<std::string>& outputs ) override;

private:
	sqlite3 * ptr;
	static const std::string CREATE;

	std::map<std::string, int> unitids;
	std::map<std::string, int> patientids;
	// unit-bed -> id
	std::map<std::pair<std::string, std::string>, int> bedids;
	// name-hz-iswave -> id
	std::map<std::tuple<std::string, double, bool>, int> signalids;

	static int nameidcb( void * a_param, int argc, char ** argv, char ** scolumn );
	static int bedcb( void * a_param, int argc, char ** argv, char ** column );
	static int signalcb( void * a_param, int argc, char ** argv, char ** column );

	void exec( const std::string& sql, int (*cb )(void*, int, char**, char**) = 0,
			void * param = nullptr );
	int getOrAddUnit( const std::string& name );
	int getOrAddBed( const std::string& name, const std::string& unitname );
	int getOrAddPatient( const std::string& name );
	int getOrAddSignal( const std::unique_ptr<SignalData>& data );
	int addLookup( const std::string& sql, const std::string& name );
	void addSignal( int fileid, const std::unique_ptr<SignalData>& sig );
	void addOffsets( int fileid, const std::unique_ptr<SignalSet>& sig );
};


#endif /* DB_H */

