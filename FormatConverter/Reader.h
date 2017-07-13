
#ifndef READER_H
#define READER_H

#include "Formats.h"

#include <map>
#include <string>
#include <memory>

#include "ReadInfo.h"

class SignalData;
class DataRow;

enum ReadResult {
	NORMAL, END_OF_PATIENT, END_OF_DAY, END_OF_FILE, ERROR
};

class Reader {
public:
	Reader( );
	virtual ~Reader( );

	static std::unique_ptr<Reader> get( const Format& fmt );

	/**
	 * Prepares for reading a new input file
	 * @param input the file
	 * @param data reset this ReadInfo as well
	 */
	virtual int reset( const std::string& input, ReadInfo& info );

	/**
	 * Reads the next chunk of data from the input file. The definition of
	 * "chunk" is left to the reader, but at a minimum, only one patient day data
	 * will ever be returned by a single call.
	 * @param read the data structure to populate with the newly-read data
	 * @return the result code
	 */
	ReadResult fill( ReadInfo& read );

protected:

	/**
	 * Reads the next bit of input and appends the data to the ReadInfo object.
	 * This function is called from {@link #fill}
	 * @param input where to store the new data
	 * @return the result code
	 */
	virtual ReadResult readChunk( ReadInfo& ) = 0;

	/**
	 * Gets a size calculation for this input
	 * @param input the input to size
	 * @return size for the input, or -1 for error
	 */
	virtual int getSize( const std::string& input ) const = 0;

	/**
	 * Prepares for reading a new file/stream. Most users will not need to call
	 * this function, as it is called from reset(). By default, does nothing
	 * @param input
	 * @param info the info object to prepare
	 * @return 0 (success), -1 (error), -2 (fatal)
	 */
	virtual int prepare( const std::string& input, ReadInfo& info );

	/**
	 * Closes the current file/stream. This function must be called when
	 * the caller is finished with a file
	 */
	virtual void finish( );

private:
	Reader( const Reader& );

	bool largefile;
};

#endif /* READER_H */
