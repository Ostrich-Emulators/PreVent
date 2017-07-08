
#ifndef FROM_READER_H
#define FROM_READER_H

#include "Formats.h"

#include <map>
#include <string>
#include <memory>

class DataSetDataCache;
class DataRow;

class FromReader {
public:
	FromReader( );
	virtual ~FromReader( );

	static std::unique_ptr<FromReader> get( const Format& fmt );

	void reset( const std::string& input );
	std::map<std::string, std::unique_ptr<DataSetDataCache>>&vitals( );
	std::map<std::string, std::unique_ptr<DataSetDataCache>>&waves( );

protected:
	void addVital( const std::string& name, const DataRow& data, const std::string& uom );
	void addWave( const std::string& name, const DataRow& data, const std::string& uom );
	virtual void doRead( const std::string& input ) = 0;
	virtual int getSize( const std::string& input ) const = 0;

private:
	FromReader( const FromReader& );

	std::map<std::string, std::unique_ptr<DataSetDataCache>> vmap;
	std::map<std::string, std::unique_ptr<DataSetDataCache>> wmap;
	bool largefile;
};

#endif /* FROM_READER_H */
