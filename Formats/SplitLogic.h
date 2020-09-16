/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SplitLogic.h
 * Author: ryan
 *
 * Created on September 16, 2020, 9:52 AM
 */

#ifndef SPLITLOGIC_H
#define SPLITLOGIC_H

#include "dr_time.h"
/** A class to encapsulate when an input file should be split into another output file*/
namespace FormatConverter {
  class SignalSet;

  class SplitLogic {
  public:
    SplitLogic( const SplitLogic& orig );
    SplitLogic& operator=(const SplitLogic& orig );
    virtual ~SplitLogic( );

    /**
     * Gets a splitter that rolls over at midnight
     * @return 
     */
    static SplitLogic midnight( );

    /**
     * Gets a splitter that never rolls
     * @return
     */
    static SplitLogic nobreaks( );

    /**
     * Gets a splitter that rolls over every X hours
     * @param numHours the number of hours to keep per rollover
     * @param clean if true, allow a partial first hour, and roll at the top of hours
     * @return the splitter
     */
    static SplitLogic hourly( int numHours, bool clean = true );

    /**
     * Decides if now is in a new duration from then.  If then is 0, this function
     * always returns false
     * @param then the old time
     * @param now the time to check
     * @return true, if now and then should be split
     */
    bool isRollover( SignalSet * data, dr_time now, bool timeIsLocal ) const;

    /**
     * Does this Splitter ever rollover?
     * @return
     */
    bool nonbreaking( ) const;
  private:
    SplitLogic( int hours = 0, bool clean = true );

    int hours; // if -1, roll at midnight
    bool clean; // when rolling by hours, the first file have a partial hour
  };
}

#endif /* SPLITLOGIC_H */

