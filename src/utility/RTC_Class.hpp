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

    /// @return true on success. On false the outputs are left untouched.
    bool getDateTime(rtc_date_t* date, rtc_time_t* time) const;
    bool getDateTime(rtc_datetime_t* datetime) const { return datetime ? getDateTime(&datetime->date, &datetime->time) : false; }
    bool getDate(rtc_date_t* date) const { return getDateTime(date, nullptr); }
    bool getTime(rtc_time_t* time) const { return getDateTime(nullptr, time); }

    /// Write the date and/or time.
    /// @param date nullptr = leave the date unchanged. If weekDay == -1 and year/month/date are set, the weekday is computed.
    /// @param time nullptr = leave the time unchanged.
    /// @return true when the RTC acknowledged every write. false when no RTC is enabled,
    ///         both arguments are nullptr, a time/month/date/weekDay field is out of range,
    ///         the year is outside what the chip stores, or on a communication failure.
    /// @note The year range differs per chip: PCF8563 1900-2099, RX8130 2000-2099, PowerHub 2000-2099.
    bool setDateTime(const tm* datetime);
    bool setDateTime(const rtc_date_t* date, const rtc_time_t* time);
    bool setDateTime(const rtc_datetime_t* datetime) { return datetime ? setDateTime(&datetime->date, &datetime->time) : false; }
    bool setDateTime(const rtc_datetime_t& datetime) { return setDateTime(&datetime.date, &datetime.time); }

    bool setDate(const rtc_date_t* date) { return setDateTime(date, nullptr); }
    bool setDate(const rtc_date_t& date) { return setDateTime(&date, nullptr); }
    bool setTime(const rtc_time_t* time) { return setDateTime(nullptr, time); }
    bool setTime(const rtc_time_t& time) { return setDateTime(nullptr, &time); }

    bool getVoltLow(void);

    /// Set (or stop) the periodic timer IRQ.
    /// @param timer_msec period in milliseconds. 0 == stop the timer.
    /// @param applied_msec optional. receives the period actually programmed (rounded to the
    ///        chip's resolution: PCF8563 1 s steps for requests below 270 s (capped at 255 s), then 1-minute steps up to 255 min; RX8130 see RX8130_Class.hpp),
    ///        or 0 when the timer was stopped. Left untouched on false.
    /// @return true when the requested state was reached (stopping included).
    ///         false when no RTC is enabled, the chip has no timer (PowerHub), or on a communication failure.
    /// @note On boards whose RTC IRQ is relayed by the M5PM1 (PaperMono, ToughC5) the stale PM1 wake
    ///       source and IRQ status are cleared as well; a failure there also returns false.
    bool setTimerIRQ(std::uint32_t timer_msec, std::uint32_t* applied_msec = nullptr);

    /// Whether the RTC provides the periodic timer (PowerHub does not).
    bool hasTimerIRQ(void) const { return _rtc_instance ? _rtc_instance->hasTimerIRQ() : false; }

    /// Whether the RTC can represent this alarm request; nothing is written.
    bool canSetAlarm(const rtc_date_t* date, const rtc_time_t* time) const { return _rtc_instance ? _rtc_instance->canSetAlarm(date, time) : false; }

    /// Set (or clear) the alarm.
    /// The alarm uses minutes, hours, date and weekDay; a field set to -1 is a wildcard.
    /// seconds, month and year are ignored (one-minute resolution). A request with no
    /// alarm field specified (setAlarmIRQ(nullptr, nullptr), or every field -1) clears
    /// the alarm and its pending flag; a null tm* is rejected and returns false.
    /// @return true when the RTC acknowledged every write and the alarm is in the requested state.
    ///         false for an invalid alarm field, when no RTC is enabled, the chip has no alarm,
    ///         or on a communication failure.
    /// @note RX8130 cannot match date and weekDay at the same time; date takes precedence.
    /// @note PowerHub: minutes and hours are required, there is no weekDay alarm
    ///       (weekDay is ignored when date is given, a weekDay-only request returns false
    ///       without touching the alarm) and day 31 cannot be set (firmware limit).
    ///       Its STM32 applies the alarm asynchronously and exposes no confirmation, so
    ///       true means the request was accepted, not that the chip has already applied it.
    /// @note See setTimerIRQ() for the M5PM1 relay note.
    /// Removed. setAlarmIRQ(seconds) programmed the periodic timer, not the alarm;
    /// use setTimerIRQ(seconds * 1000). Deleted (instead of just removed) so that
    /// setAlarmIRQ(0) fails to compile rather than silently binding to the tm* overload.
    bool setAlarmIRQ(int afterSeconds) = delete;

    bool setAlarmIRQ(const tm* datetime);
    bool setAlarmIRQ(const rtc_date_t* date, const rtc_time_t* time);
    bool setAlarmIRQ(const rtc_date_t &date, const rtc_time_t &time) { return setAlarmIRQ(&date, &time); }
    bool setAlarmIRQ(const rtc_time_t &time) { return setAlarmIRQ(nullptr, &time); }

    void setSystemTimeFromRtc(struct timezone* tz = nullptr);

    /// @return true while the timer or alarm flag is raised. false when no RTC is enabled, unsupported, or unreadable.
    bool getIRQstatus(void);

    /// Clear the timer and alarm flags (the IRQ line is released).
    /// @return true when the flags were cleared, or when the chip keeps no host-visible flags (PowerHub).
    ///         false when no RTC is enabled or on a communication failure.
    /// @note On boards whose RTC IRQ is relayed by the M5PM1, its stale wake source
    ///       and IRQ status are cleared as well; a failure there also returns false.
    bool clearIRQ(void);

    /// Stop the timer and disable the alarm, then clear their flags.
    /// @return true when the IRQ sources are disabled. false when no RTC is enabled, unsupported, or on a communication failure.
    /// @note PowerHub: its STM32 applies the change asynchronously, so true means the request was accepted.
    /// @note On boards whose RTC IRQ is relayed by the M5PM1, its stale wake source
    ///       and IRQ status are cleared as well; a failure there also returns false.
    bool disableIRQ(void);

    /// Convenience getters. On failure every field is -1; check with isValid().
    rtc_time_t getTime(void) const
    {
      rtc_time_t time;
      if (!getTime(&time)) { time = rtc_time_t { -1, -1, -1 }; }
      return time;
    }

    rtc_date_t getDate(void) const
    {
      rtc_date_t date;
      if (!getDate(&date)) { date = rtc_date_t { -1, -1, -1, -1 }; }
      return date;
    }

    rtc_datetime_t getDateTime(void) const
    {
      rtc_datetime_t res;
      if (!getDateTime(&res)) { res = rtc_datetime_t { rtc_date_t { -1, -1, -1, -1 }, rtc_time_t { -1, -1, -1 } }; }
      return res;
    }
  };
}

#endif
