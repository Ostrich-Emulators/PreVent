
#ifndef WRITER_H
#define WRITER_H

#include <utility>
#include <memory>
#include <vector>

#include "Formats.h"
#include "ReadInfo.h"

class Reader;

class Writer {
public:
	Writer( );
	virtual ~Writer( );

	static std::unique_ptr<Writer> get( const Format& fmt );

	void setOutputDir( const std::string& outdir );
	void setOutputPrefix( const std::string& pre );
	void setCompression( int lev );

	virtual std::vector<std::string> write( std::unique_ptr<Reader>& from,
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
	virtual int drain( ReadInfo& ) = 0;

private:
	Writer( const Writer& );

	std::string outdir;
	std::string prefix;
	int compression;
};

#endif /* WRITER_H */
