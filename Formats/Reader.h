
#ifndef READER_H
#define READER_H

#include "Formats.h"

#include <map>
#include <string>
#include <memory>
#include <sstream>

#include "dr_time.h"
#include "SignalSet.h"

class SignalData;
class DataRow;

enum ReadResult {
  FIRST_READ = 0,
  NORMAL = 1,
  END_OF_PATIENT = 2,
  END_OF_DAY = 3,
  END_OF_FILE = 4,
  ERROR = 5
};

class Reader {
public:
  Reader( const std::string& name );
  virtual ~Reader( );

  static std::unique_ptr<Reader> get( const Format& fmt );

  /**
   * Prepares for reading a new input file/stream.
   * @param input the file
   * @param data reset this ReadInfo as well
   * @return 0 (success), -1 (error), -2 (fatal)
   */
  virtual int prepare( const std::string& input, std::unique_ptr<SignalSet>& info );

  /**
   * Closes the current file/stream. This function must be called when
   * the caller is finished with a file
   */
  virtual void finish( );

  /**
   * Fills the given ReadInfo with the next chunk of data from the input.
   * A chunk is defined as one patient-day.
   * @param read the data structure to populate with the newly-read data
   * @param lastresult the outcome of the previous call to fill()
   * @return the result code
   */
  virtual ReadResult fill( std::unique_ptr<SignalSet>& read,
      const ReadResult& lastresult = ReadResult::FIRST_READ ) = 0;

  /**
   *  Gets a user-friendly name for this reader
   * @return a name to put in the attributes of any SignalSet created by
   * this reader
   */
  virtual std::string name( ) const;

  void setQuiet( bool quiet = true );
  void setNonbreaking( bool nb = false );
  void localizeTime( bool local = true );
  bool localizingTime( ) const;

  static void strptime2( const std::string& input, const std::string& format,
      std::tm * tm );


  /**
   * Gets root attributes from the given input. If this can be accomplished
   * without reading the whole file, do it. else set the ok parameter to false
   * @param inputfile
   * @param map the map to fill with attributes
   * @return true, if the read occurred
   */
  virtual bool getAttributes( const std::string& inputfile, std::map<std::string, std::string>& map );

protected:
  Reader( const Reader& );

  /**
   * Gets a size calculation for this input
   * @param input the input to size
   * @return size for the input, or 0 for error
   */
  virtual size_t getSize( const std::string& input ) const = 0;

  bool nonbreaking( ) const;

  std::ostream& output( ) const;
private:

  bool largefile;
  const std::string rdrname;
  bool quiet;
  bool anon;
  bool onefile;
  bool local_time;
  std::stringstream ss;
};

#endif /* READER_H */
