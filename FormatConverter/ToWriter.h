
#ifndef TO_WRITER_H
#define TO_WRITER_H

#include <utility>
#include <memory>
#include <vector>

#include "Formats.h"
#include "ReadInfo.h"

class FromReader;

class ToWriter {
public:
	ToWriter( );
	virtual ~ToWriter( );

	static std::unique_ptr<ToWriter> get( const Format& fmt );

	void setOutputDir( const std::string& outdir );
	void setOutputPrefix( const std::string& pre );
	void setCompression( int lev );

	virtual std::vector<std::string> write( std::unique_ptr<FromReader>& from,
			ReadInfo& data );

protected:
	/**
	 * Initializes a new (possibly temporary) data file
	 * @param newfile
	 * @param compression
	 * @return
	 */
	virtual void initDataSet( const std::string& newfile, int compression ) = 0;

	/**
	 * Closes the current data file, and provides the final name for it. Datafiles
	 * can change names during writing, so only this function provides the name
	 * of the actual, final, file location
	 * @return 
	 */
	virtual std::string closeDataSet( ) = 0;
	virtual int writeChunk( ReadInfo& ) = 0;

private:
	ToWriter( const ToWriter& );

	std::string outdir;
	std::string prefix;
	int compression;
};

#endif /* TO_WRITER_H */
