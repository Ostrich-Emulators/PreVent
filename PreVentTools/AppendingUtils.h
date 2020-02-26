/* 
 * File:   AppendingUtils.h
 * Author: rpb6eg
 *
 * Created on February 26, 2020, 9:11 AM
 */

#ifndef APPENDINGUTILS_H
#define APPENDINGUTILS_H

#include <string>
#include <H5Cpp.h>
#include <memory>

namespace FormatConverter {
  class SignalData;

  /**
   * Utilities for appending data to an HDF5 file.
   */
  class AppendingUtils {
  public:
    static const std::string DATAGROUP;

    AppendingUtils( const std::string& targetfile );
    virtual ~AppendingUtils( );

    /**
     * Appends the data from the given datafile to the targetfile. The targetfile
     * has the format:
     * <pre>
     * NAME=calculated data
int prop:i=183
dbl prop:d=37.3
str prop1=this is a string prop
str prop2:s=this is also a string prop
Columns=raw value
DATA=i
182
1928
12
9587
7364
31
43
4
7
3422
...</pre>
     *
     * The first line must be the NAME=...
     * The only other required line is DATA=...
     *
     * @param datafile
     * @return 0 if success, anything else is a failure
     */
    int append( const std::string& datafile );

  private:
    std::string filename;
    H5::H5File file;

    void ensureCustomDataGroupExists( );
    static std::unique_ptr<SignalData> parseDataFile( const std::string& datafile );
    static void split( const std::string& line, std::string& key, std::string& val,
        const std::string& delim = "=" );
    int writeSignal( std::unique_ptr<SignalData>& signal );
  };
}


#endif /* APPENDINGUTILS_H */

