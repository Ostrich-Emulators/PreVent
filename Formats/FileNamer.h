/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FileNamer.h
 * Author: ryan
 * class for naming output files
 * Created on March 21, 2018, 11:49 AM
 */

#ifndef FILENAMER_H
#define FILENAMER_H

#include <memory>
#include <string>
#include <map>
#include <set>
#include "dr_time.h"

namespace FormatConverter {
  class SignalSet;

  class FileNamer {
  public:
    static const std::string DEFAULT_PATTERN;
    static const std::string FILENAME_PATTERN;

    virtual ~FileNamer( );
    FileNamer& operator=(const FileNamer& orig );
    FileNamer( const FileNamer& orig );

    static FileNamer parse( const std::string& pattern );

    void tofmt( const std::string& ext );
    void patientOrdinal( int patientnum );
    void fileOrdinal( int filenum );
    void inputfilename( const std::string& input );
    std::string inputfilename( ) const;

    void allowDuplicates( bool allow = true );

    /**
     * Provides a filename (including directory) for the given SignalData and
     * output file ordinal (and patient ordinal, if set)
     * @param data
     * @param outputnum
     * @return the filename
     * @throws an runtime_error if the generated filename is a repeat and dupesOk is false
     */
    std::string filename( SignalSet * data );

    /**
     * Gets a filename (including directory) without an extension (or the preceding .)
     * @param data
     * @param outputnum
     * @return
     */
    std::string filenameNoExt( SignalSet * data );
    /**
     * Gets a filename (including directory) based on whatever information we
     * already have. Some conversions cannot be performed with this function,
     * so use the other one whenever possible
     * @return
     */
    std::string filename( );

    std::string filenameNoExt( );
    /**
     * What was the last-generated filename?
     * @return
     */
    std::string last( ) const;

    static std::string getDateSuffix( const tm * date, const std::string& sep = "-" );

    static std::string YYYYMMDD( const tm * time );
    static std::string HHmmss( const tm * time );

  private:
    FileNamer( const std::string& pat );
    std::string pattern;
    std::map<std::string, std::string> conversions;
    std::string lastname;
    std::string inputfile;
    bool dupesOk;
    std::set<std::string> oldnames;

    static tm modtime( const dr_time& time );
  };
}
#endif /* FILENAMER_H */

