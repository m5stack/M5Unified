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

    /// true when every field is within range (hours 0-23, minutes 0-59, seconds 0-59).
    /// Use it on values returned by RTC_Class::getTime()/getDateTime(), which set
    /// every field to -1 on failure (the pointer getters leave their output untouched
    /// and must be checked through their return value). A time passed to setAlarmIRQ()
    /// may legitimately hold -1 (wildcard), so this is not a precondition for the setters.
    bool isValid(void) const
    {
      return (std::uint8_t)hours < 24 && (std::uint8_t)minutes < 60 && (std::uint8_t)seconds < 60;
    }
  };

  struct __attribute__((packed)) rtc_date_t
  {
    /// year (the range an RTC can store depends on the chip; see RTC_Class::setDateTime)
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

    /// true when every field is within range (year >= 0, month 1-12, date 1-31, weekDay 0-6).
    /// Calendar consistency (e.g. February 30) is not checked, and the year range an
    /// RTC can store is narrower than this; see the setter notes.
    /// Use it on values returned by RTC_Class::getDate()/getDateTime(), which set every
    /// field to -1 on failure (the pointer getters leave their output untouched and must
    /// be checked through their return value). A date passed to setDateTime() may hold
    /// weekDay -1 (auto-computed) and one passed to setAlarmIRQ() may hold -1 wildcards,
    /// so this is not a precondition for the setters.
    bool isValid(void) const
    {
      return year >= 0 && (std::uint8_t)(month - 1) < 12 && (std::uint8_t)(date - 1) < 31 && (std::uint8_t)weekDay < 7;
    }
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

    /// true when both date and time are valid. See rtc_date_t::isValid().
    bool isValid(void) const { return date.isValid() && time.isValid(); }
  };

  class RTC_Base : public I2C_Device
  {
  public:
    RTC_Base(std::uint8_t i2c_addr, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}
    virtual ~RTC_Base(void) = default;  // deleted through std::unique_ptr<RTC_Base>

    virtual bool begin(I2C_Class* i2c = nullptr) = 0;

    /// @return true on success. On false the outputs are left untouched.
    virtual bool getDateTime(rtc_date_t* date = nullptr, rtc_time_t* time = nullptr) const = 0;

    /// @param date nullptr = leave the date registers unchanged. weekDay == -1 is auto-computed by RTC_Class;
    ///        the drivers themselves require every field to be in range (see validateDateTime).
    /// @param time nullptr = leave the time registers unchanged.
    /// @return true when every register write was acknowledged.
    ///         false for a nullptr pair, a year the chip cannot store, or a communication failure.
    virtual bool setDateTime(const rtc_date_t* const date = nullptr, const rtc_time_t* const time = nullptr) = 0;

    /// Set (or stop) the periodic timer IRQ.
    /// @param timer_msec period in milliseconds. 0 == stop the timer.
    /// @param applied_msec optional. receives the period actually programmed (rounded to the
    ///        chip's resolution), or 0 when the timer was stopped. Left untouched on false.
    /// @return true when the requested state was reached (stopping included).
    ///         false on a communication failure (the driver then stops the timer on a
    ///         best-effort basis) or when the chip has no timer.
    virtual bool setTimerIRQ(std::uint32_t timer_msec, std::uint32_t* applied_msec = nullptr) { (void)timer_msec; (void)applied_msec; return false; }

    /// Set (or clear) the alarm.
    /// The alarm uses minutes, hours, date and weekDay; a field set to -1 is a wildcard.
    /// seconds, month and year are ignored. A request with no alarm field specified
    /// clears the alarm and its pending flag.
    /// On a chip driven through a front-end that applies requests asynchronously (PowerHub)
    /// true means the front-end accepted the request.
    /// @return true when every register write was acknowledged and the alarm is in the
    ///         requested state (enabled or cleared). false for an invalid alarm field,
    ///         on a communication failure, or when the chip has no alarm.
    virtual bool setAlarmIRQ(const rtc_date_t *date, const rtc_time_t *time) { (void)date; (void)time; return false; }

    /// Whether the chip provides the periodic timer used by setTimerIRQ(). A driver
    /// that implements setTimerIRQ() overrides this to true.
    virtual bool hasTimerIRQ(void) const { return false; }

    /// Whether this driver can represent the alarm request (fields in range, and no
    /// feature the chip lacks). Checked by RTC_Class before any register is touched.
    /// A driver that implements setAlarmIRQ() overrides this (validateAlarmFields() at least).
    virtual bool canSetAlarm(const rtc_date_t *date, const rtc_time_t *time) const { (void)date; (void)time; return false; }

    /// @return true while the timer or alarm flag is raised. false when unsupported or unreadable.
    virtual bool getIRQstatus(void) { return false; }

    /// Clear the timer and alarm flags (the IRQ line is released).
    /// @return true when the flags were cleared, or when the chip keeps no host-visible flags (PowerHub).
    ///         false on a communication failure.
    virtual bool clearIRQ(void) { return false; }

    /// Stop the timer and disable the alarm, then clear their flags.
    /// @return true when the IRQ sources are disabled. false on a communication failure or when unsupported.
    ///         Behind an asynchronous front-end (PowerHub) true means the request was accepted.
    virtual bool disableIRQ(void) { return false; }

    virtual bool getVoltLow(void) { return false; }

  protected:
    /// Check that both nibbles of a (masked) register byte are valid BCD.
    /// A corrupted I2C read (e.g. 0xFF) would otherwise decode to a
    /// plausible-looking but impossible value such as year 2165.
    static bool isValidBcd(std::uint8_t value)
    {
      return ((value & 0x0F) <= 9) && ((value >> 4) <= 9);
    }

    static std::uint8_t bcd2ToByte(std::uint8_t value)
    {
      return ((value >> 4) * 10) + (value & 0x0F);
    }

    static std::uint8_t byteToBcd2(std::uint8_t value)
    {
      std::uint_fast8_t bcdhigh = value / 10;
      return (bcdhigh << 4) | (value - (bcdhigh * 10));
    }

    /// Range-check decoded values before committing them to the output.
    static bool validateDateTime(const rtc_date_t* date, const rtc_time_t* time);

  public:
    /// Validate the fields used by the alarm registers. -1 is the only wildcard;
    /// seconds, month, and year are intentionally ignored.
    static bool validateAlarmFields(const rtc_date_t* date, const rtc_time_t* time);
  };
}

#endif
