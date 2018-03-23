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
#include "dr_time.h"

class SignalSet;

class FileNamer {
public:
  static const std::string DEFAULT_PATTERN;
  static const std::string FILENAME_PATTERN;

  virtual ~FileNamer( );
  FileNamer& operator=(const FileNamer& orig );
  FileNamer( const FileNamer& orig );

  static FileNamer parse( const std::string& pattern );

  void outputdir( const std::string& dir );
  std::string outputdir() const;
  void tofmt( const std::string& ext );
  void patientOrdinal( int patientnum );
  void inputfilename( const std::string& input );

  /**
   * Provides a filename (including directory) for the given SignalData and
   * output file ordinal (and patient ordinal, if set)
   * @param data
   * @param outputnum
   * @return
   */
  std::string filename( const SignalSet& data, int outputnum = -1 );

  /**
   * Gets a filename (including directory) without an extension (or the preceding .)
   * @param data
   * @param outputnum
   * @return
   */
  std::string filenameNoExt( const SignalSet& data, int outputnum = -1 );
  /**
   * Gets a filename (including directory) based on whatever information we
   * already have. Some conversions cannot be performed with this function,
   * so use the other one whenever possible
   * @return
   */
  std::string filename();

  std::string filenameNoExt();
  /**
   * What was the last-generated filename?
   * @return 
   */
  std::string last( ) const;

  static std::string getDateSuffix( const dr_time& date, const std::string& sep = "-" );

private:
  FileNamer( const std::string& pat );
  int patientnum;
  std::string pattern;
  std::map<std::string, std::string> conversions;
  std::string lastname;
};


#endif /* FILENAMER_H */

