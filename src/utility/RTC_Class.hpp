// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_RTC_CLASS_H__
#define __M5_RTC_CLASS_H__

#include "rtc/RTC_Base.hpp"
#include "m5unified_common.h"

#include "I2C_Class.hpp"
#include <memory>

namespace m5
{
  class RTC_Class
  {
    // インスタンス保持用
    std::unique_ptr<RTC_Base> _rtc_instance;
  public:

    bool begin(I2C_Class* i2c = nullptr, board_t board = board_t::board_unknown);
    bool init(I2C_Class* i2c = nullptr) { return begin(i2c); }

    bool isEnabled(void) const { return _rtc_instance.get() != nullptr; }

    bool getVoltLow(void);

    bool getTime(rtc_time_t* time) const;
    bool getDate(rtc_date_t* date) const;
    bool getDateTime(rtc_datetime_t* datetime) const;

    void setTime(const rtc_time_t &time);
    void setTime(const rtc_time_t* const time) { if (time) { setTime(*time); } }

    void setDate(rtc_date_t date);
    void setDate(const rtc_date_t* const date) { if (date) { setDate(*date); } }

    void setDateTime(const rtc_datetime_t &datetime) { setDate(datetime.date); setTime(datetime.time); }
    void setDateTime(const rtc_datetime_t* const datetime) { if (datetime) { setDateTime(*datetime); } }
    void setDateTime(const tm* const datetime)
    {
      if (datetime)
      {
        rtc_datetime_t dt { *datetime };
        setDateTime(dt);
      }
    }

    /// Set timer IRQ
    /// @param afterSeconds 1 - 15,300. If 256 or more, 1-minute cycle.  (max 255 minute.)
    /// @return the set number of seconds.
    int setAlarmIRQ(int afterSeconds);

    /// Set alarm by time
    int setAlarmIRQ(const rtc_time_t &time);
    int setAlarmIRQ(const rtc_date_t &date, const rtc_time_t &time);

    void setSystemTimeFromRtc(struct timezone* tz = nullptr);

    bool getIRQstatus(void);
    void clearIRQ(void);
    void disableIRQ(void);

    rtc_time_t getTime(void) const
    {
      rtc_time_t time;
      getTime(&time);
      return time;
    }

    rtc_date_t getDate(void) const
    {
      rtc_date_t date;
      getDate(&date);
      return date;
    }

    rtc_datetime_t getDateTime(void) const
    {
      rtc_datetime_t res;
      getDateTime(&res);
      return res;
    }

  };
}

#endif
