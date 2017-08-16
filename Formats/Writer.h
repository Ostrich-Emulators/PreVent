
#ifndef WRITER_H
#define WRITER_H

#include <utility>
#include <memory>
#include <vector>

#include "Formats.h"
#include "SignalSet.h"

class Reader;

class Writer {
public:
	Writer( );
	virtual ~Writer( );

	static std::unique_ptr<Writer> get( const Format& fmt );

	void setOutputDir( const std::string& outdir );
	void setOutputPrefix( const std::string& pres );
	void setCompression( int lev );

	virtual std::vector<std::string> write( std::unique_ptr<Reader>& from,
			SignalSet& data );

protected:
	static std::string getDateSuffix( const time_t& date, const std::string& sep = "-" );

	/**
	 * Initializes a new (possibly temporary) data file
	 * @param newfile
	 * @param compression
	 * @return 0 (Success), -1 (Error)
	 */
	virtual int initDataSet( const std::string& outdir, const std::string& prefix,
			int compression ) = 0;

	/**
	 * Closes the current data file, and provides the final name for it. Datafiles
	 * can change names during writing, so only this function provides the name
	 * of the actual, final, file location. Also, a single dataset can be written
	 * to multiple files
	 * @return 
	 */
	virtual std::vector<std::string> closeDataSet( ) = 0;

	/**
	 * Drains the give ReadInfo's data. This function can be used for incrementally
	 * writing input data, or essentially ignored until closeDataSet is called.
	 * @param info The data to drain
	 * @return 0 (success) -1 (error)
	 */
	virtual int drain( SignalSet& info ) = 0;

private:
	Writer( const Writer& );

	std::string outdir;
	std::string prefix;
	int compression;
};

#endif /* WRITER_H */
