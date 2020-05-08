/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ZlReader2.h
 * Author: ryan
 *
 */

#ifndef STPPHOILIPSREADER_H
#define STPPHOILIPSREADER_H

#include "StpReaderBase.h"

namespace FormatConverter {
  class SignalData;
  class StreamChunkReader;

  /**
   * A reader for the HSDI signal files that UVA is producing
   */
  class StpPhilipsReader : public StpReaderBase {
  public:
    StpPhilipsReader( const std::string& name = "STP (Philips)" );
    virtual ~StpPhilipsReader( );
    static std::vector<StpReaderBase::StpMetadata> parseMetadata( const std::string& input );

  protected:
    virtual ReadResult fill( std::unique_ptr<SignalSet>&, const ReadResult& lastfill ) override;
    virtual int prepare( const std::string& input, std::unique_ptr<SignalSet>& info ) override;

  private:
    static const std::string XML_HEADER;
    static const std::string LT;
    static const std::string GT;

    static StpMetadata metaFromSignalSet( const std::unique_ptr<SignalSet>& );

    enum ParseState {
      UNINTERESTING, PATIENTINFO, NUMERICCOMPOUND, NUMERICVALUE, NUMERICATTR,
      WAVE
    };

    /**
     * Skips forward in the work buffer until the needle is at the head
     * @param needle
     */
    void skipUntil( const std::string& needle );

    enum ChunkReadResult {
      OK, ROLLOVER, NEW_PATIENT, UNKNOWN_BLOCKTYPE, HR_BLOCK_PROBLEM
    };

    static const std::map<int, std::string> WAVELABELS;

    StpPhilipsReader( const StpPhilipsReader& orig );

    /**
     * determines if the work buffer has at least one full XML doc in it
     * @param doc a complete XML doc, if it exists, will be appended to this string
     * @param rootelement the root element of the document will be appended to this string
     * @return
     */
    bool hasCompleteXmlDoc( std::string& doc, std::string& rootelement );
    dr_time parseTime( const std::string_view& datetime );
    static std::string_view peekPatientId( const std::string& xmldoc, bool ispatientdoc );
    dr_time peekTime( const std::string_view& xmldoc );

    ParseState state;
    dr_time currentTime;
    bool wavewarning;

    struct xmlpassthru {
      std::unique_ptr<SignalSet>& signals;
      ParseState state;
      std::string currentText;
      std::string currentPatientId;
      std::string label;
      std::string value;
      std::string uom;
      std::string sampleperiod;
      dr_time currentTime;
      StpPhilipsReader& outer;
    };

    class PatientParser {
    public:
      static void start( void * data, const char * el, const char ** attr );
      static void end( void * data, const char * el );
      static void chars( void * data, const char * text, int len );
    private:
      static std::string xmltext;
    };

    class DataParser {
    public:
      static void start( void * data, const char * el, const char ** attr );
      static void end( void * data, const char * el );
      static void chars( void * data, const char * text, int len );
    private:
      static const std::string NUMERIC_CMPD;
      static const std::string NUMERIC_ATTR;
      static const std::string NUMERIC_VAL;
      static const std::string WAVE;
      static const std::string WAVE_SEG;
    };
  };
}
#endif /* STPPHOILIPSREADER_H */

