// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_IMPLEMENTATION
#error "RTC_Class.inl is part of M5Unified.cpp and is not meant to be included on its own"
#endif

#include "../M5Unified.hpp"
#include "RTC_Class.hpp"

#include "m5unified_common.h"

#if !defined(M5UNIFIED_PC_BUILD)

#include <sdkconfig.h>

#endif

#include "rtc/RTC_Base.hpp"
#include "rtc/RTC_PowerHub_Class.hpp"
#include "rtc/PCF8563_Class.hpp"
#include "rtc/RX8130_Class.hpp"

namespace m5
{
#if defined (CONFIG_IDF_TARGET_ESP32S3)
  static bool clear_m5pm1_rtc_irq(void)
  {
    if (M5.getBoard() == board_t::board_M5PaperMono
     && M5.Power.getType() == Power_Class::pmic_t::pmic_m5pm1)
    {
      bool res = M5.Power.M5pm1.clearWakeSource();
      return M5.Power.M5pm1.clearIRQStatus() && res;
    }
    return true;
  }
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
  static bool clear_m5pm1_rtc_irq(void)
  {
    if (M5.getBoard() == board_t::board_M5ToughC5
     && M5.Power.getType() == Power_Class::pmic_t::pmic_m5pm1)
    {
      bool res = M5.Power.M5pm1.clearWakeSource();
      return M5.Power.M5pm1.clearIRQStatus() && res;
    }
    return true;
  }
#else
  static bool clear_m5pm1_rtc_irq(void) { return true; }
#endif

  bool RTC_Class::begin(I2C_Class* i2c, board_t board)
  {
    if (i2c)
    {
      i2c->begin();
    }

    auto instance = std::unique_ptr<RTC_Base>();
    switch (board)
    {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
      case board_t::board_M5CoreP4X:
      case board_t::board_M5Tab5:
      case board_t::board_M5Tab5X:
        instance.reset(new RX8130_Class(RX8130_Class::DEFAULT_ADDRESS, 400000, i2c));
        break;
#endif

#if defined (CONFIG_IDF_TARGET_ESP32C5)
      case board_t::board_M5ToughC5:
        instance.reset(new RX8130_Class(RX8130_Class::DEFAULT_ADDRESS, 400000, i2c));
        break;
#endif

#if defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5PowerHub:
        instance.reset(new RTC_PowerHub_Class(RTC_PowerHub_Class::DEFAULT_ADDRESS, 400000));
        break;

      case board_t::board_M5StopWatch:
      case board_t::board_M5StampPLC:
      case board_t::board_M5PaperColor:
      case board_t::board_M5PaperMono:
      case board_t::board_M5ChainCaptain:
        instance.reset(new RX8130_Class(RX8130_Class::DEFAULT_ADDRESS, 400000, i2c));
        break;
#endif

      default:
        break;
    }

    if (instance == nullptr)
    {
      instance.reset(new PCF8563_Class(PCF8563_Class::DEFAULT_ADDRESS, 400000, i2c));
    }

    if (instance->begin())
    {
      _rtc_instance = std::move(instance);
      return true;
    }

    _rtc_instance.reset();
    return false;
  }

  bool RTC_Class::getVoltLow(void)
  {
    return _rtc_instance ? _rtc_instance->getVoltLow() : false;
  }

  bool RTC_Class::getDateTime(rtc_date_t* date, rtc_time_t* time) const
  {
    return _rtc_instance ? _rtc_instance->getDateTime(date, time) : false;
  }

  // Range-check the int fields of a tm before they are narrowed to int8_t/int16_t.
  // A full date/time: every field must be valid, tm_wday may be -1 (computed later).
  static bool tm_datetime_in_range(const tm& t)
  {
    return t.tm_sec >= 0 && t.tm_sec <= 59 && t.tm_min >= 0 && t.tm_min <= 59
        && t.tm_hour >= 0 && t.tm_hour <= 23 && t.tm_mday >= 1 && t.tm_mday <= 31
        && t.tm_mon >= 0 && t.tm_mon <= 11 && t.tm_wday >= -1 && t.tm_wday <= 6
        && t.tm_year >= -1900 && t.tm_year <= 32767 - 1900;
  }

  bool RTC_Class::setDateTime(const tm* datetime)
  {
    if (!datetime || !tm_datetime_in_range(*datetime)) { return false; }
    rtc_datetime_t dt { *datetime };
    return setDateTime(&dt.date, &dt.time);
  }

  bool RTC_Class::setDateTime(const rtc_date_t* date, const rtc_time_t* time)
  {
    if (!_rtc_instance || (!date && !time)) { return false; }
    if (time && !time->isValid()) { return false; }
    if (date && (date->year < 0
              || (std::uint8_t)(date->month - 1) >= 12
              || (std::uint8_t)(date->date - 1) >= 31
              || date->weekDay < -1 || date->weekDay > 6))
    {
      return false;
    }

    rtc_date_t date_local;
    if (date && date->weekDay < 0)
    {
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
      int32_t weekDay = (year + (year >> 2) - ydiv100 + (ydiv100 >> 2) + (13 * month + 8) / 5 + day) % 7;
      date_local.weekDay = weekDay < 0 ? weekDay + 7 : weekDay;
    }
    return _rtc_instance->setDateTime(date, time);
  }

  bool RTC_Class::setTimerIRQ(std::uint32_t timer_msec, std::uint32_t* applied_msec)
  {
    if (!_rtc_instance) { return false; }
    // Order: stop the old timer, clear the stale PM1 wake source / IRQ status,
    // then arm the new timer. Stopping first means no event of the old timer
    // can re-latch the relay after it was cleared, and clearing before arming
    // means an event of the new timer is never wiped as stale.
    bool stopped = _rtc_instance->setTimerIRQ(0, nullptr);
    bool relay_cleared = clear_m5pm1_rtc_irq();  // attempted even when the stop failed
    if (!stopped || !relay_cleared) { return false; }
    if (timer_msec == 0) { if (applied_msec) { *applied_msec = 0; } return true; }
    std::uint32_t applied_local = 0;
    if (!_rtc_instance->setTimerIRQ(timer_msec, &applied_local)) { return false; }
    if (applied_msec) { *applied_msec = applied_local; }
    return true;
  }

  bool RTC_Class::setAlarmIRQ(const tm* datetime)
  {
    if (!datetime) { return false; }
    // Only the alarm fields are taken from the tm; -1 stays a wildcard and the
    // ignored fields (seconds, month, year) are not looked at. The range check
    // is done on the int values before they are narrowed.
    const tm& t = *datetime;
    auto in_range = [](int v, int max) { return v == -1 || (v >= 0 && v <= max); };
    if (!in_range(t.tm_min, 59) || !in_range(t.tm_hour, 23) || !in_range(t.tm_wday, 6)
     || !(t.tm_mday == -1 || (t.tm_mday >= 1 && t.tm_mday <= 31)))
    {
      return false;
    }
    rtc_time_t time { (std::int8_t)t.tm_hour, (std::int8_t)t.tm_min, -1 };
    rtc_date_t date { -1, -1, (std::int8_t)t.tm_mday, (std::int8_t)t.tm_wday };
    return setAlarmIRQ(&date, &time);
  }

  bool RTC_Class::setAlarmIRQ(const rtc_date_t* date, const rtc_time_t* time)
  {
    if (!_rtc_instance) { return false; }
    // Rejected before anything is touched when the driver cannot represent the request.
    if (!_rtc_instance->canSetAlarm(date, time)) { return false; }
    // Same order as setTimerIRQ: clear the old alarm, clear the stale PM1 state, arm.
    bool cleared = _rtc_instance->setAlarmIRQ(nullptr, nullptr);
    bool relay_cleared = clear_m5pm1_rtc_irq();  // attempted even when the clear failed
    if (!cleared || !relay_cleared) { return false; }
    bool clear_only = !((time && (time->minutes >= 0 || time->hours >= 0))
                     || (date && (date->date >= 0 || date->weekDay >= 0)));
    return clear_only ? true : _rtc_instance->setAlarmIRQ(date, time);
  }

  bool RTC_Class::getIRQstatus(void)
  {
    return _rtc_instance ? _rtc_instance->getIRQstatus() : false;
  }

  bool RTC_Class::clearIRQ(void)
  {
    if (!_rtc_instance) { return false; }
    bool res = _rtc_instance->clearIRQ();
    return clear_m5pm1_rtc_irq() && res;
  }

  bool RTC_Class::disableIRQ(void)
  {
    if (!_rtc_instance) { return false; }
    bool res = _rtc_instance->disableIRQ();
    return clear_m5pm1_rtc_irq() && res;
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
