
#include "Log.h"

namespace FormatConverter{

  LogLevel Log::_level = LogLevel::DEBUG;

  nullstream Log::silencer;

  void Log::setlevel( LogLevel l ) {
    _level = l;
  }

  LogLevel Log::level( ) {
    return _level;
  }

  std::ostream& Log::debug( ) {
    return levelstream( LogLevel::DEBUG );
  }

  std::ostream& Log::levelstream( LogLevel l ) {
    return (static_cast<int>( _level ) >= static_cast<int>( l )
        ? std::cout
        : silencer );
  }
}
