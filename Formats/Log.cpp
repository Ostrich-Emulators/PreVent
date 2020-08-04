
#include "Log.h"

namespace FormatConverter{

  LogLevel Log::_level = LogLevel::INFO;

  nullstream Log::silencer;

  void Log::setlevel( LogLevel l ) {
    _level = l;
  }

  LogLevel Log::level( ) {
    return _level;
  }

  std::ostream& Log::error( ) {
    return ( levelok( LogLevel::ERROR )
        ? std::cerr
        : silencer );
  }

  std::ostream& Log::warn( ) {
    return levelstream( LogLevel::WARN );
  }

  std::ostream& Log::info( ) {
    return levelstream( LogLevel::INFO );
  }

  std::ostream& Log::debug( ) {
    return levelstream( LogLevel::DEBUG );
  }

  std::ostream& Log::trace( ) {
    return levelstream( LogLevel::TRACE );
  }

  std::ostream& Log::out( ) {
    return levelstream( LogLevel::ALL );
  }

  bool Log::levelok( LogLevel l ) {
    //    std::cout << "checking " << static_cast<int> ( _level ) << " >= " << static_cast<int> ( l )
    //        << "? " << ( static_cast<int> ( _level ) >= static_cast<int> ( l ) ) << std::endl;

    return (static_cast<int> ( _level ) >= static_cast<int> ( l ) );
  }

  std::ostream& Log::levelstream( LogLevel l ) {
    return (levelok( l )
        ? std::cout
        : silencer );
  }
}
