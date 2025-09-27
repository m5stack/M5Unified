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
    RTC_Base* getRtcInstancePtr(void) const { return _rtc_instance.get(); }

    bool getDateTime(rtc_date_t* date, rtc_time_t* time) const;
    bool getDateTime(rtc_datetime_t* datetime) const { return datetime ? getDateTime(&datetime->date, &datetime->time) : false; }
    bool getDate(rtc_date_t* date) const { return getDateTime(date, nullptr); }
    bool getTime(rtc_time_t* time) const { return getDateTime(nullptr, time); }

    void setDateTime(const tm* datetime);
    void setDateTime(const rtc_date_t* date, const rtc_time_t* time);
    void setDateTime(const rtc_datetime_t* datetime) { setDateTime(&datetime->date, &datetime->time); };
    void setDateTime(const rtc_datetime_t& datetime) { setDateTime(&datetime.date, &datetime.time); }

    void setDate(const rtc_date_t* date) { setDateTime(date, nullptr); }
    void setDate(const rtc_date_t& date) { setDateTime(&date, nullptr); }
    void setTime(const rtc_time_t* time) { setDateTime(nullptr, time); }
    void setTime(const rtc_time_t& time) { setDateTime(nullptr, &time); }
    
    bool getVoltLow(void);

    /// Set timer IRQ
    /// @param timer_msec
    /// @return the set number of msec. (0 == disable)
    std::uint32_t setTimerIRQ(std::uint32_t timer_msec);

    // deprecated
    /// @param afterSeconds 1 - 15,300. If 256 or more, 1-minute cycle.  (max 255 minute.)
    /// @return the set number of seconds.
    int setAlarmIRQ(int afterSeconds) { return setTimerIRQ(afterSeconds * 1000) / 1000; }

    /// Set alarm by time
    int setAlarmIRQ(const tm* datetime);
    int setAlarmIRQ(const rtc_date_t* date, const rtc_time_t* time);
    int setAlarmIRQ(const rtc_date_t &date, const rtc_time_t &time) { return setAlarmIRQ(&date, &time); }
    int setAlarmIRQ(const rtc_time_t &time) { return setAlarmIRQ(nullptr, &time); }

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
