#pragma once

#include <iostream>

#include "exports.h"

namespace FormatConverter {

  class nullbuff : public std::streambuf {
  public:

    int overflow( int c ) {
      return c;
    }
  };

  class nullstream : public std::ostream {
  public:

    nullstream( ) : std::ostream( &buffer ) { }
  private:
    nullbuff buffer;
  };

  enum class FMTCNV_EXPORT LogLevel : int {
    NONE, // NONE has to be the first value
    ERROR,
    WARN,
    INFO,
    DEBUG,
    TRACE,
    ALL // ALL has to be the last value in this enum
  };

  class Log {
  public:
    static FMTCNV_EXPORT void setlevel( LogLevel l );
    static FMTCNV_EXPORT LogLevel level( );

    static FMTCNV_EXPORT std::ostream& error( );
    static FMTCNV_EXPORT std::ostream& warn( );
    static FMTCNV_EXPORT std::ostream& info( );
    static FMTCNV_EXPORT std::ostream& debug( );
    static FMTCNV_EXPORT std::ostream& trace( );
    static FMTCNV_EXPORT std::ostream& out( );

    static FMTCNV_EXPORT bool levelok( LogLevel l );
  private:
    static std::ostream& levelstream( LogLevel l );
    static LogLevel _level;
    static nullstream silencer;
  };
}
