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
NAME=calculated data
int prop:i=183
dbl prop:d=37.3
str prop1=this is a string prop
str prop2:s=this is also a string prop
Columns=raw value
DATA
1270638517000,182.5
1270638519000,1928
1270638521000,12
1270638523000,9587
1270638525000,7364
1270638527000,31
1270638529000,43
1270638531000,4
1270638533000,7
1270638535000,3422
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

