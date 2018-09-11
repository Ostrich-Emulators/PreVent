
/* 
 * File:   LocaltimeSignalSet.h
 * Author: ryan
 *
 * Created on July 2, 2018, 3:16 PM
 */

#ifndef TIMEZONEOFFSETSIGNALSET_H
#define TIMEZONEOFFSETSIGNALSET_H

#include "OffsetTimeSignalSet.h"

/**
 * A class to dynamically set timezone information and offset
 */

class TimezoneOffsetTimeSignalSet : public OffsetTimeSignalSet {
public:
  static const std::string COLLECTION_TIMEZONE;
  TimezoneOffsetTimeSignalSet();
  TimezoneOffsetTimeSignalSet( const std::unique_ptr<SignalSet>& w);
  TimezoneOffsetTimeSignalSet( SignalSet * w );
  ~TimezoneOffsetTimeSignalSet( );

  virtual void reset( bool signalDataOnly = false ) override;
  virtual void complete() override;

private:
  std::string tz_name;
  bool tz_calculated;
};

#endif /* TIMEZONEOFFSETSIGNALSET_H */

