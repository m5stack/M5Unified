// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_RTC_BASE_H__
#define __M5_RTC_BASE_H__

#include "../I2C_Class.hpp"

#if __has_include(<sys/time.h>)
#include <sys/time.h>
#else
typedef void timezone;
#endif
#include <time.h>

namespace m5
{
  struct __attribute__((packed)) rtc_time_t
  {
    std::int8_t hours;
    std::int8_t minutes;
    std::int8_t seconds;

    rtc_time_t(std::int8_t hours_ = -1, std::int8_t minutes_ = -1, std::int8_t seconds_ = -1)
    : hours   { hours_   }
    , minutes { minutes_ }
    , seconds { seconds_ }
    {}

    rtc_time_t(const tm& t)
    : hours   { (int8_t)t.tm_hour }
    , minutes { (int8_t)t.tm_min  }
    , seconds { (int8_t)t.tm_sec  }
    {}
  };

  struct __attribute__((packed)) rtc_date_t
  {
    /// year 1900-2099
    std::int16_t year;

    /// month 1-12
    std::int8_t month;

    /// date 1-31
    std::int8_t date;

    /// weekDay 0:sun / 1:mon / 2:tue / 3:wed / 4:thu / 5:fri / 6:sat
    std::int8_t weekDay;

    rtc_date_t(std::int16_t year_ = 2000, std::int8_t month_ = 1, std::int8_t date_ = -1, std::int8_t weekDay_ = -1)
    : year    { year_    }
    , month   { month_   }
    , date    { date_    }
    , weekDay { weekDay_ }
    {}

    rtc_date_t(const tm& t)
    : year    { (int16_t)(t.tm_year + 1900) }
    , month   { (int8_t )(t.tm_mon  + 1   ) }
    , date    { (int8_t ) t.tm_mday         }
    , weekDay { (int8_t ) t.tm_wday         }
    {}
  };

  struct __attribute__((packed)) rtc_datetime_t
  {
    rtc_date_t date;
    rtc_time_t time;
    rtc_datetime_t() = default;
    rtc_datetime_t(const rtc_date_t& d, const rtc_time_t& t) : date { d }, time { t } {};
    rtc_datetime_t(const tm& t) : date { t }, time { t } {}
    tm get_tm(void) const;
    void set_tm(tm& time);
    void set_tm(tm* t) { if (t) set_tm(*t); }
  };

  class RTC_Base : public I2C_Device
  {
  public:
    RTC_Base(std::uint8_t i2c_addr, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    virtual bool begin(I2C_Class* i2c = nullptr) = 0;

    virtual bool getDateTime(rtc_date_t* date = nullptr, rtc_time_t* time = nullptr) const = 0;
    virtual bool setDateTime(const rtc_date_t* const date = nullptr, const rtc_time_t* const time = nullptr) = 0;

    /// Set timer IRQ
    /// @param milliseconds (0 == disable)
    /// @return the set number of milliseconds. (0 == disable)
    virtual std::uint32_t setTimerIRQ(std::uint32_t timer_msec) { return 0; };

    /// Set alarm by time
    virtual int setAlarmIRQ(const rtc_date_t *date, const rtc_time_t *time) { return 0; }

    virtual bool getIRQstatus(void) { return false; }
    virtual void clearIRQ(void) {};
    virtual void disableIRQ(void) {};

    virtual bool getVoltLow(void) { return false; }
  };
}

#endif
