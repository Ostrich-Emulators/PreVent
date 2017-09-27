
#ifndef READER_H
#define READER_H

#include "Formats.h"

#include <map>
#include <string>
#include <memory>

#include "SignalSet.h"

class SignalData;
class DataRow;

enum ReadResult {
  NORMAL, END_OF_PATIENT, END_OF_DAY, END_OF_FILE, ERROR
};

class Reader {
public:
  Reader();
  virtual ~Reader();

	static const std::string MISSING_VALUESTR;


  static std::unique_ptr<Reader> get(const Format& fmt);

  /**
   * Prepares for reading a new input file/stream.
   * @param input the file
   * @param data reset this ReadInfo as well
   * @return 0 (success), -1 (error), -2 (fatal)
   */
  virtual int prepare(const std::string& input, SignalSet& info);

  /**
   * Closes the current file/stream. This function must be called when
   * the caller is finished with a file
   */
  virtual void finish();

  /**
   * Tells this reader to only produce data for the given dataset during 
   * calls to {@link #fill}.
   * @param toexport
   */
  virtual void extractOnly(const std::string& toExtract);

  /**
   * Fills the given ReadInfo with the next chunk of data from the input.
   * A chunk is defined as one patient-day.
   * @param read the data structure to populate with the newly-read data
   * @param lastresult the outcome of the previous call to fill()
   * @return the result code
   */
  virtual ReadResult fill(SignalSet& read,
          const ReadResult& lastresult = ReadResult::NORMAL) = 0;

protected:

  /**
   * Gets a size calculation for this input
   * @param input the input to size
   * @return size for the input, or 0 for error
   */
  virtual size_t getSize(const std::string& input) const = 0;

  /**
   * Should we extract this waveform/vital?
   * @return 
   */
  bool shouldExtract( const std::string& vitalOrWave ) const;
  
private:
  Reader(const Reader&);

  bool largefile;
  std::string toextract;
};

#endif /* READER_H */
