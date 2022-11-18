/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CsvReader.h
 * Author: ryan
 *
 * Created on September 1, 2020, 9:23 PM
 */

#ifndef CSVREADER_H
#define CSVREADER_H

#include "Reader.h"

#include <vector>
#include <string>
#include <fstream>
#include "dr_time.h"

namespace FormatConverter {

  class CsvReader : public Reader {
  public:
    /**
     * Constructs a new instance.
     * @param name
     * @param firstlineIsHeader
     * @param timef which field contains the timestamp? Note that if this is bigger than the
     *   fieldcount, 0 will be used instead
     * @param fieldcount sets the field count. Note that this will be calculated (and overwritten)
     *   in the prepare call if firstLineIsHeader is true.
     */
    CsvReader( const std::string& name = "CSV", bool firstlineIsHeader = true, int timef = 0,
        int fieldcount = -1 );
    virtual ~CsvReader( );

  protected:
    virtual int prepare( const std::string& input, SignalSet * info ) override;
    ReadResult fill( SignalSet * data, const ReadResult& lastfill ) override;

    virtual dr_time converttime( const std::string& timeline );
    virtual std::string headerForField( int field, const std::vector<std::string>& linevals ) const;
    virtual bool includeFieldValue( int field, const std::vector<std::string>& linevals ) const;
    virtual bool isNewPatient( const std::vector<std::string>& linevals, SignalSet * info ) const;
    virtual void setMetas( const std::vector<std::string>& linevals, SignalSet * data );

    bool isTimeField( int field ) const;

  private:
    /**
     * split the line into the given number of values, and decode the time field into the timer
     * @param csvline the text line
     * @param timer the time (out) to set for returned values
     * @return the values
     */
    virtual std::vector<std::string> linevalues( const std::string& csvline, dr_time & timer );
    void loadMetas( SignalSet * info );

    const bool firstLineIsHeader;
    std::ifstream datafile;
    std::vector<std::string> metadata;
    std::vector<std::string> headings;
    int timefield;
    int fieldcount;
  };
}

#endif /* CSVREADER_H */

