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
    NONE = 0, DEBUG = 1
  };

  class Log {
  public:
    static FMTCNV_EXPORT void setlevel( LogLevel l );
    static FMTCNV_EXPORT std::ostream& debug( );
    static FMTCNV_EXPORT LogLevel level( );
  private:
    static std::ostream& levelstream( LogLevel l );
    static LogLevel _level;
    static nullstream silencer;
  };
}
