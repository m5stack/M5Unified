// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_IMPLEMENTATION
#error "RTC_PowerHub_Class.inl is part of M5Unified.cpp and is not meant to be included on its own"
#endif

#include "RTC_PowerHub_Class.hpp"
#include <algorithm>
#include <stdlib.h>

namespace m5
{
  // Weekday round trip through the PowerHub firmware:
  // the firmware writes bin2bcd(reg) into the RX8130 WDAY register, which is a
  // one-hot bit (bit N = weekday N). Writing the decimal values 1,2,4,8,10,20,40
  // makes bin2bcd() produce 0x01,0x02,0x04,0x08,0x10,0x20,0x40. On readback,
  // looking up those exposed decimal values recovers weekday 0-6. The registers
  // themselves hold plain binary, not BCD, for every other field.
  static std::uint8_t weekdayToPowerHub(std::int8_t weekDay)
  {
    static constexpr std::uint8_t weekDayTable[] = { 1, 2, 4, 8, 10, 20, 40 };
    return ((std::uint8_t)weekDay < sizeof(weekDayTable)) ? weekDayTable[weekDay] : 0;
  }

  static bool powerHubToWeekday(std::uint8_t value, std::int8_t* weekDay)
  {
    static constexpr std::uint8_t weekDayTable[] = { 1, 2, 4, 8, 10, 20, 40 };
    for (std::size_t i = 0; i < sizeof(weekDayTable); ++i)
    {
      if (weekDayTable[i] == value)
      {
        *weekDay = (std::int8_t)i;
        return true;
      }
    }
    return false;
  }

  // Register map of the STM32 firmware (I2C address 0x50).
  static constexpr std::uint8_t REG_WAKE_SRC     = 0xB0; // nSTBY_WKUP enable, stored in flash: write only when needed
  static constexpr std::uint8_t REG_RTC_SEC      = 0xC0; // C0..C6: sec, min, hour, mday, mon, year-2000, wday
  static constexpr std::uint8_t REG_RTC_MDAY     = 0xC3;
  static constexpr std::uint8_t REG_ALARM_MIN    = 0xD0; // D0..D2: min, hour, mday (0 = any day)
  static constexpr std::uint8_t REG_ALARM_ENABLE = 0xD3;
  static constexpr std::uint8_t REG_FW_VERSION   = 0xFE;
  // The STM32 collects D0..D3 writes behind one "update pending" bit that its
  // main loop (about 13 ms per iteration) applies to the RX8130 and then clears.
  // A write that lands while an older update is being applied can be dropped
  // with that clear, so every alarm-register write is followed by a pause long
  // enough for the loop to consume it before the next one is issued.
  static constexpr std::uint32_t ALARM_APPLY_WAIT_MS = 50;
  // The firmware applies D0..D3 to the RX8130 from its main loop; there is no
  // reliable "applied" flag (0xD4 of firmware 0xF3 mirrors other registers), so
  // success means the firmware acknowledged the register write.

  bool RTC_PowerHub_Class::begin(I2C_Class* i2c)
  {
    if (i2c)
    {
      _i2c = i2c;
      i2c->begin();
    }
    std::uint8_t ver = 0;
    _init = readRegister(REG_FW_VERSION, &ver, 1);
    return _init;
  }

  bool RTC_PowerHub_Class::getVoltLow(void)
  {
    // The firmware does not expose the RX8130 VBLF flag.
    return false;
  }

  bool RTC_PowerHub_Class::getDateTime(rtc_date_t* date, rtc_time_t* time) const
  {
    std::uint8_t buf[7] = { 0 };
    int start_reg = (time != nullptr) ? REG_RTC_SEC : REG_RTC_MDAY;
    int len = ((time != nullptr) ? 3 : 0) + ((date != nullptr) ? 4 : 0);
    if (!isEnabled() || (date == nullptr && time == nullptr) || !readRegister(start_reg, buf, len))
    {
      return false;
    }

    // Decode into locals and validate before committing (see PCF8563).
    int idx = 0;
    rtc_time_t t;
    if (time)
    {
      t.seconds = buf[idx++];
      t.minutes = buf[idx++];
      t.hours   = buf[idx++];
    }

    rtc_date_t d;
    if (date)
    {
      d.date  = buf[idx++];
      d.month = buf[idx++];
      std::uint8_t year = buf[idx++];
      std::uint8_t weekDay = buf[idx++];
      if (year > 99 || !powerHubToWeekday(weekDay, &d.weekDay)) { return false; }
      d.year = year + 2000;
    }

    if (!validateDateTime(date ? &d : nullptr, time ? &t : nullptr))
    {
      return false;
    }
    if (time) { *time = t; }
    if (date) { *date = d; }
    return true;
  }

  bool RTC_PowerHub_Class::setDateTime(const rtc_date_t* date, const rtc_time_t* time)
  {
    if (!isEnabled() || !validateDateTime(date, time)) { return false; }
    // The year register holds 0-99 (2000-2099).
    if (date && (date->year < 2000 || date->year > 2099)) { return false; }
    std::uint8_t buf[7] = { 0 };
    int idx = 0;
    int reg_start = REG_RTC_MDAY;

    if (time)
    {
      reg_start = REG_RTC_SEC;
      buf[idx++] = time->seconds;
      buf[idx++] = time->minutes;
      buf[idx++] = time->hours;
    }

    if (date)
    {
      buf[idx++] = date->date;
      buf[idx++] = date->month;
      buf[idx++] = date->year - 2000;
      buf[idx++] = weekdayToPowerHub(date->weekDay);
    }

    if (idx == 0) { return false; }
    return writeRegister(reg_start, buf, idx);
  }

  bool RTC_PowerHub_Class::setTimerIRQ(std::uint32_t timer_msec, std::uint32_t* applied_msec)
  {
    // The firmware exposes no periodic timer.
    (void)timer_msec; (void)applied_msec;
    return false;
  }

  bool RTC_PowerHub_Class::canSetAlarm(const rtc_date_t *date, const rtc_time_t *time) const
  {
    if (!validateAlarmFields(date, time)) { return false; }
    // No weekday alarm: weekDay is ignored when a date is given (as on RX8130),
    // a weekDay-only request cannot be represented.
    if (date && date->weekDay >= 0 && date->date < 0) { return false; }
    // The firmware accepts an alarm day only below 31 (0 = every day); 31 would be
    // dropped and the previous value (0 after our clear) would match every day.
    if (date && date->date == 31) { return false; }
    // No minute/hour wildcard: a request with a date but no complete time cannot be represented.
    bool time_given = time && (time->minutes >= 0 || time->hours >= 0);
    bool date_given = date && date->date >= 0;
    if ((time_given || date_given) && (!time_given || time->minutes < 0 || time->hours < 0)) { return false; }
    return true;
  }

  bool RTC_PowerHub_Class::writeAlarmRegisters(std::uint8_t reg, const std::uint8_t* data, std::size_t length)
  {
    bool result = writeRegister(reg, data, length);
    m5gfx::delay(ALARM_APPLY_WAIT_MS);
    return result;
  }

  bool RTC_PowerHub_Class::setAlarmIRQ(const rtc_date_t *date, const rtc_time_t *time)
  {
    if (!canSetAlarm(date, time) || !isEnabled()) { return false; }

    // buf: alarm min, hour, mday, enable. All zero = clear the alarm.
    std::uint8_t buf[4] = { 0, 0, 0, 0 };
    bool time_given = time && (time->minutes >= 0 || time->hours >= 0);
    bool date_given = date && date->date >= 0;
    if (time_given || date_given)
    {
      // The firmware has no minute/hour wildcard and no weekday alarm, so a
      // request it cannot represent is refused without touching the registers.
      if (!time_given || time->minutes < 0 || time->hours < 0) { return false; }
      buf[0] = time->minutes;
      buf[1] = time->hours;
      buf[2] = (date && date->date >= 0) ? date->date : 0;  // 0 = match every day
      buf[3] = 1;
    }

    // Disable the old alarm before changing any value register: after a failed
    // write nothing is armed (the previous alarm values are not re-enabled).
    bool disabled = false;
    const std::uint8_t disable = 0;
    for (int retry = 0; retry < 3 && !disabled; ++retry)
    {
      disabled = writeAlarmRegisters(REG_ALARM_ENABLE, &disable, 1);
    }
    if (!disabled) { return false; }

    if (buf[3])
    {
      // Allow the RX8130 /INT to wake the STM32 from standby. The setting lives
      // in flash, so it is written only when it is not already set.
      std::uint8_t wake = 0;
      if (!readRegister(REG_WAKE_SRC, &wake, 1)) { return false; }
      if (wake == 0 && !writeRegister8(REG_WAKE_SRC, 1)) { return false; }
      // Do not roll B0 back after this point: a retained wake permission is
      // harmless, whereas restoring a flash-backed setting risks a new failure.
    }

    // Values and the enable flag go out in one transaction. The STM32 collects
    // every D0..D3 write behind a single "update pending" flag that its main loop
    // applies to the RX8130 and then clears; an enable that arrives while an older
    // update is being applied can be lost, so the whole alarm is handed over as
    // one register-block write.
    if (!writeAlarmRegisters(REG_ALARM_MIN, buf, 4))
    {
      writeAlarmRegisters(REG_ALARM_ENABLE, &disable, 1);
      return false;
    }
    return true;
  }

  bool RTC_PowerHub_Class::getIRQstatus(void)
  {
    // The alarm flag is consumed by the STM32; the firmware does not expose it.
    return false;
  }

  bool RTC_PowerHub_Class::clearIRQ(void)
  {
    // The flags are managed by the STM32 and are not visible from the host,
    // so there is nothing for the host to clear.
    return isEnabled();
  }

  bool RTC_PowerHub_Class::disableIRQ(void)
  {
    if (!isEnabled()) { return false; }
    const std::uint8_t disable = 0;
    return writeAlarmRegisters(REG_ALARM_ENABLE, &disable, 1);
  }
}
