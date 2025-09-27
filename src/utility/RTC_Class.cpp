// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "../M5Unified.hpp"
#include "RTC_Class.hpp"

#include "m5unified_common.h"

#if !defined(M5UNIFIED_PC_BUILD)

#include <sdkconfig.h>

#endif

#include "rtc/PCF8563_Class.hpp"
#include "rtc/RX8130_Class.hpp"

namespace m5
{
  bool RTC_Class::begin(I2C_Class* i2c, board_t board)
  {
    if (i2c)
    {
      i2c->begin();
    }

    bool result = false;

#if defined (CONFIG_IDF_TARGET_ESP32P4)
    if (result == false && board == board_t::board_M5Tab5)
    {
      auto instance = new RX8130_Class( RX8130_Class::DEFAULT_ADDRESS, 400000, i2c );
      result = instance->begin();
      _rtc_instance.reset(instance);
    }
#endif

    if (result == false)
    {
      auto instance = new PCF8563_Class( PCF8563_Class::DEFAULT_ADDRESS, 400000, i2c );
      result = instance->begin();
      _rtc_instance.reset(instance);
    }

    if (result == false)
    {
      _rtc_instance.reset();
    }
    return result;
  }

  bool RTC_Class::getVoltLow(void)
  {
    return _rtc_instance ? _rtc_instance->getVoltLow() : false;
  }

  bool RTC_Class::getDateTime(rtc_date_t* date, rtc_time_t* time) const
  {
    return _rtc_instance ? _rtc_instance->getDateTime(date, time) : false;
  }

  void RTC_Class::setDateTime(const tm* datetime)
  {
    if (datetime)
    {
      rtc_datetime_t dt { *datetime };
      setDateTime(&dt.date, &dt.time);
    }
  }

  void RTC_Class::setDateTime(const rtc_date_t* date, const rtc_time_t* time)
  {
    if (!_rtc_instance) { return; }

    rtc_date_t date_local;
    if (date)
    {
      std::uint8_t w = date->weekDay;
      if (w > 6 && date->year >= 1900 && ((std::size_t)(date->month - 1)) < 12)
      { /// weekDay auto adjust
        date_local = *date;
        date = &date_local;
        int32_t year = date_local.year;
        int32_t month = date_local.month;
        int32_t day = date_local.date;
        if (month < 3) {
          year--;
          month += 12;
        }
        int32_t ydiv100 = year / 100;
        date_local.weekDay = (year + (year >> 2) - ydiv100 + (ydiv100 >> 2) + (13 * month + 8) / 5 + day) % 7;
      }
    }
    _rtc_instance->setDateTime(date, time);
  }

  std::uint32_t RTC_Class::setTimerIRQ(std::uint32_t timer_msec)
  {
    return _rtc_instance ? _rtc_instance->setTimerIRQ(timer_msec) : 0;
  }

  int RTC_Class::setAlarmIRQ(const tm* datetime)
  {
    if (datetime)
    {
      rtc_datetime_t dt { *datetime };
      return setAlarmIRQ(&dt.date, &dt.time);
    }
    return -1;
  }

  int RTC_Class::setAlarmIRQ(const rtc_date_t* date, const rtc_time_t* time)
  {
    return _rtc_instance ? _rtc_instance->setAlarmIRQ(date, time) : -1;
  }

  bool RTC_Class::getIRQstatus(void)
  {
    return _rtc_instance ? _rtc_instance->getIRQstatus() : false;
  }

  void RTC_Class::clearIRQ(void)
  {
    if (!_rtc_instance) { return; }
    _rtc_instance->clearIRQ();
  }

  void RTC_Class::disableIRQ(void)
  {
    if (!_rtc_instance) { return; }
    _rtc_instance->disableIRQ();
  }

  void RTC_Class::setSystemTimeFromRtc(struct timezone* tz)
  {
#if defined (M5UNIFIED_PC_BUILD)
    (void)tz;
#else
    rtc_datetime_t dt;
    if (getDateTime(&dt))
    {
      tm t_st;
      t_st.tm_isdst = -1;
      t_st.tm_year = dt.date.year - 1900;
      t_st.tm_mon  = dt.date.month - 1;
      t_st.tm_mday = dt.date.date;
      t_st.tm_hour = dt.time.hours;
      t_st.tm_min  = dt.time.minutes;
      t_st.tm_sec  = dt.time.seconds;
      timeval now;
      // mktime(3) uses localtime, force UTC
      char *oldtz = getenv("TZ");
      setenv("TZ", "GMT0", 1);
      tzset(); // Workaround for https://github.com/espressif/esp-idf/issues/11455
      now.tv_sec = mktime(&t_st);
      if (oldtz)
      {
        setenv("TZ", oldtz, 1);
      } else {
        unsetenv("TZ");
      }
      now.tv_usec = 0;
      settimeofday(&now, tz);
    }
#endif
  }
}
