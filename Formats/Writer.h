
#ifndef WRITER_H
#define WRITER_H

#include <utility>
#include <memory>
#include <vector>
#include <ostream>
#include <sstream>

#include "Formats.h"
#include "ConversionListener.h"

namespace FormatConverter {
  class Reader;
  class FileNamer;
  class SignalSet;

  class Writer {
  public:
    virtual ~Writer( );
    static const int DEFAULT_COMPRESSION;
    static std::unique_ptr<Writer> get( const FormatConverter::Format& fmt );

    void compression( int lev );
    int compression( ) const;
    void addListener( std::shared_ptr<ConversionListener> listener );
    void quiet( bool = true );
    void stopAfterFirstFile( bool onlyone = true );
    void filenamer( const FormatConverter::FileNamer& namer );
    FormatConverter::FileNamer& filenamer( ) const;
    const std::string& ext( ) const;

    virtual std::vector<std::string> write( std::unique_ptr<Reader>& from,
        std::unique_ptr<SignalSet>& data );

  protected:
    Writer( const std::string& extension );
    std::ostream& output( ) const;

    /**
     * Lets subclasses initialize a new (possibly temporary) data file.
     * By default, does nothing
     * @return 0 (Success), -1 (Error)
     */
    virtual int initDataSet( );

    /**
     * Closes the current data file, and provides the final name for it. Datafiles
     * can change names during writing, so only this function provides the name
     * of the actual, final, file location. Also, a single dataset can be written
     * to multiple files
     * @return 
     */
    virtual std::vector<std::string> closeDataSet( ) = 0;

    /**
     * Drains the give ReadInfo's data. This function can be used for incrementally
     * writing input data, or essentially ignored until closeDataSet is called.
     * @param info The data to drain
     * @return 0 (success) -1 (error)
     */
    virtual int drain( std::unique_ptr<SignalSet>& info ) = 0;

    int tz_offset( ) const;
    const std::string& tz_name( ) const;

  private:
    Writer( const Writer& );

    std::vector<std::shared_ptr<ConversionListener>> listeners;
    int compress;
    bool bequiet;
    bool testrun;
    std::stringstream ss;
    const std::string extension; // filename extension
    std::unique_ptr<FormatConverter::FileNamer> namer;
    int gmt_offset;
    std::string timezone;
  };
}
#endif /* WRITER_H */
