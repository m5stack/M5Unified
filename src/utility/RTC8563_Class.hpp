// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_RTC8563_CLASS_H__
#define __M5_RTC8563_CLASS_H__

#include "I2C_Class.hpp"

#include <time.h>
#include <sys/time.h>

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
  };

  struct __attribute__((packed)) rtc_datetime_t
  {
    rtc_date_t date;
    rtc_time_t time;
  };

  class RTC8563_Class : public I2C_Device
  {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x51;

    RTC8563_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    bool begin(I2C_Class* i2c = nullptr);

    bool getVoltLow(void);

    bool getTime(rtc_time_t* time) const;
    bool getDate(rtc_date_t* date) const;
    bool getDateTime(rtc_datetime_t* datetime) const;

    void setTime(const rtc_time_t &time);
    void setTime(const rtc_time_t* const time) { if (time) { setTime(*time); } }

    void setDate(const rtc_date_t &date);
    void setDate(const rtc_date_t* const date) { if (date) { setDate(*date); } }

    void setDateTime(const rtc_datetime_t &datetime) { setDate(datetime.date); setTime(datetime.time); }
    void setDateTime(const rtc_datetime_t* const datetime) { if (datetime) { setDateTime(*datetime); } }
    void setDateTime(const tm* const datetime)
    {
      if (datetime)
      {
        setDate( { (int16_t)(datetime->tm_year + 1900)
                 , (int8_t)(datetime->tm_mon + 1)
                 , (int8_t)(datetime->tm_mday)
                 , (int8_t)(datetime->tm_wday)
                 } );
        setTime( { (int8_t)(datetime->tm_hour)
                 , (int8_t)(datetime->tm_min)
                 , (int8_t)(datetime->tm_sec)
                 } );
      }
    }

    /// Set timer IRQ
    /// @param afterSeconds 1 - 15,300. If 256 or more, 1-minute cycle.  (max 255 minute.)
    /// @return the set number of seconds.
    int setAlarmIRQ(int afterSeconds);

    /// Set alarm by time
    int setAlarmIRQ(const rtc_time_t &time);
    int setAlarmIRQ(const rtc_date_t &date, const rtc_time_t &time);

    void setSystemTimeFromRtc(timezone* tz = nullptr);

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
