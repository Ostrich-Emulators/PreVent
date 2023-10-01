/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Options.h
 * Author: ryan
 *
 * Created on February 12, 2019, 7:26 PM
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <map>
#include "dr_time.h"

namespace FormatConverter {

  enum OptionsKey {
    NOCACHE,
    INDEXED_TIME,
    ANONYMIZE,
    LOCALIZED_TIME,
    NO_BREAK,
    SKIP_WAVES,
    TMPDIR,
    SKIP_UNTIL_DATETIME
  };

  class Options {
  public:
    virtual ~Options( );

    static bool isset( OptionsKey key );

    static void set( OptionsKey key, const std::string& val );
    static void set( OptionsKey key, bool val = true );
    static void set( OptionsKey key, const dr_time& time );
    static std::string get( OptionsKey key );
    static bool asBool( OptionsKey key );
    static dr_time asTime( OptionsKey key );

  private:
    Options( );

    static std::map<OptionsKey, std::string> map;

  };
}
#endif /* OPTIONS_H */

