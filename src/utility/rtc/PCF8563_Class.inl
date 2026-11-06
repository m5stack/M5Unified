// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_IMPLEMENTATION
#error "PCF8563_Class.inl is part of M5Unified.cpp and is not meant to be included on its own"
#endif

#include "PCF8563_Class.hpp"

#include <stdlib.h>

namespace m5
{
  bool PCF8563_Class::begin(I2C_Class* i2c)
  {
    if (i2c)
    {
      _i2c = i2c;
      i2c->begin();
    }
    /// TimerCameraの内蔵RTCが初期化に失敗することがあったため、最初に空打ちする; 
    writeRegister8(0x00, 0x00);
    _init = writeRegister8(0x00, 0x00) && writeRegister8(0x0E, 0x03);
    return _init;
  }

  bool PCF8563_Class::getVoltLow(void)
  {
    return readRegister8(0x02) & 0x80; // RTCC_VLSEC_MASK
  }

  bool PCF8563_Class::getDateTime(rtc_date_t* date, rtc_time_t* time) const
  {
    std::uint8_t buf[7] = { 0 };
    int start_reg = (time != nullptr) ? 0x02 : 0x05;
    int len = ((date != nullptr) ? 4 : 0)
            + ((time != nullptr) ? 3 : 0);
    if (!isEnabled() || len == 0 || !readRegister(start_reg, buf, len))
    {
      return false;
    }

    // Decode into locals and validate before committing, so that a corrupted
    // register value (I2C glitch, uninitialized RTC) results in false instead
    // of a bogus date such as 45:00:80 or year 2165.
    int idx = 0;
    rtc_time_t t;
    if (time)
    {
      std::uint8_t sec  = buf[idx++] & 0x7f;
      std::uint8_t min  = buf[idx++] & 0x7f;
      std::uint8_t hour = buf[idx++] & 0x3f;
      if (!isValidBcd(sec) || !isValidBcd(min) || !isValidBcd(hour))
      {
        return false;
      }
      t.seconds = bcd2ToByte(sec);
      t.minutes = bcd2ToByte(min);
      t.hours   = bcd2ToByte(hour);
    }

    rtc_date_t d;
    if (date)
    {
      std::uint8_t dd = buf[idx++] & 0x3f;
      std::uint8_t wd = buf[idx++] & 0x07;
      std::uint8_t mo = buf[idx] & 0x1f;
      bool century    = buf[idx++] & 0x80;
      std::uint8_t yy = buf[idx];
      if (!isValidBcd(dd) || !isValidBcd(mo) || !isValidBcd(yy))
      {
        return false;
      }
      d.date    = bcd2ToByte(dd);
      d.weekDay = wd;
      d.month   = bcd2ToByte(mo);
      d.year    = bcd2ToByte(yy) + (century ? 1900 : 2000);
    }

    if (!validateDateTime(date ? &d : nullptr, time ? &t : nullptr))
    {
      return false;
    }
    if (time) { *time = t; }
    if (date) { *date = d; }
    return true;
  }

  bool PCF8563_Class::setDateTime(const rtc_date_t* date, const rtc_time_t* time)
  {
    if (!_init || !validateDateTime(date, time)) { return false; }
    // The century bit covers 1900-2099 only.
    if (date && (date->year < 1900 || date->year > 2099)) { return false; }
    std::uint8_t buf[7] = { 0 };

    int idx = 0;
    int reg_start = 0x05;
    if (time)
    {
      reg_start = 0x02;
      buf[idx++] = byteToBcd2(time->seconds);
      buf[idx++] = byteToBcd2(time->minutes);
      buf[idx++] = byteToBcd2(time->hours);
    }

    if (date)
    {
      buf[idx++] = byteToBcd2(date->date);
      buf[idx++] = (uint8_t)(0x07u & date->weekDay);
      buf[idx++] = (std::uint8_t)(byteToBcd2(date->month) + (date->year < 2000 ? 0x80 : 0));
      buf[idx++] = byteToBcd2(date->year % 100);
    }
    if (idx == 0) { return false; }
    return writeRegister(reg_start, buf, idx);
  }

  bool PCF8563_Class::setTimerIRQ(std::uint32_t msec, std::uint32_t* applied_msec)
  {
    if (!_init) { return false; }
    std::uint8_t reg_value = 0;
    if (!readRegister(0x01, &reg_value, 1))
    {
      // Best effort: stop the timer even when Control_status_2 is unreadable.
      // The retries are deliberately independent so TIE is still cleared when
      // either transaction fails. A retry that can read Control_status_2 writes
      // AF/TF as 1 so a flag raised between the read and write is preserved.
      writeRegister8(0x0E, 0x03);
      std::uint8_t retry_value = 0;
      if (readRegister(0x01, &retry_value, 1))
      {
        writeRegister8(0x01, (retry_value & 0x12) | 0x0C);
      }
      else
      { // Still unreadable: the state is unknown, so release the IRQ line
        // (TIE off, TF cleared) even though AIE cannot be preserved.
        writeRegister8(0x01, 0x08);
      }
      return false;
    }
    // AF/TF are W0C. Clear TF only: writing AF=1 retains it even if it latches
    // after this read. Keep TI_TP/AIE/TIE and always write the N bits as zero.
    reg_value = (reg_value & 0x13) | 0x08;

    // Stop: timer control off (0x0E bit7 = 0) first, then TIE off with TF
    // cleared, so a TF raised by a last tick cannot survive the stop.
    auto stop_timer = [this, reg_value](void) -> bool {
      bool ok = writeRegister8(0x0E, 0x03);
      return writeRegister8(0x01, reg_value & ~0x01) && ok;
    };

    std::uint32_t afterSeconds = msec / 1000 + ((msec % 1000) >= 500); // round to nearest without overflowing near UINT32_MAX
    if (msec && !afterSeconds) { afterSeconds = 1; }
    if (afterSeconds <= 0)
    {
      if (!stop_timer()) { return false; }
      if (applied_msec) { *applied_msec = 0; }
      return true;
    }

    std::size_t div = 1;
    std::uint8_t type_value = 0x82;
    if (afterSeconds < 270)
    {
      if (afterSeconds > 255) { afterSeconds = 255; }
    }
    else
    {
      div = 60;
      afterSeconds = (afterSeconds + 30) / div;
      if (afterSeconds > 255) { afterSeconds = 255; }
      type_value = 0x83;
    }

    // Stop the running timer first (TIE off, TE off, TF cleared), load the
    // counter, then start it and enable TIE last, so no IRQ of the old period
    // can fire while the new one is being programmed.
    bool ok = stop_timer()
           && writeRegister8(0x0F, afterSeconds)
           && writeRegister8(0x0E, type_value)
           && writeRegister8(0x01, (reg_value | 0x01) & ~0x80);
    if (!ok)
    {
      stop_timer();  // best effort; the state is unknown if this fails too
      return false;
    }
    if (applied_msec) { *applied_msec = afterSeconds * div * 1000; }
    return true;
  }

  bool PCF8563_Class::setAlarmIRQ(const rtc_date_t *date, const rtc_time_t *time)
  {
    if (!validateAlarmFields(date, time) || !_init) { return false; }
    // Control_status_2 AF/TF are W0C, so never use a generic RMW to only
    // change AIE: a flag can latch between its read and write.  Writing 1 to
    // AF/TF preserves either flag; N bits are always written as 0.
    auto disable_alarm_irq = [this](void) -> bool {
      std::uint8_t control2 = 0;
      return readRegister(0x01, &control2, 1)
          && writeRegister8(0x01, (control2 & 0x11) | 0x0C);
    };
    union
    {
      std::uint32_t raw = ~0;
      std::uint8_t buf[4];
    };

    bool irq_enable = false;
    if (time) {
      if (time->minutes >= 0)
      {
        irq_enable = true;
        buf[0] = byteToBcd2(time->minutes) & 0x7f;
      }

      if (time->hours >= 0)
      {
        irq_enable = true;
        buf[1] = byteToBcd2(time->hours) & 0x3f;
      }
    }
    if (date) {
      if (date->date >= 0)
      {
        irq_enable = true;
        buf[2] = byteToBcd2(date->date) & 0x3f;
      }

      if (date->weekDay >= 0)
      {
        irq_enable = true;
        buf[3] = byteToBcd2(date->weekDay) & 0x07;
      }
    }

    // Keep AIE off until every alarm register has been written successfully.
    // The initial disable is retried: if it never succeeds the old alarm stays armed.
    bool disabled = false;
    for (int retry = 0; retry < 3 && !disabled; ++retry) { disabled = disable_alarm_irq(); }
    if (!disabled) { return false; }
    if (!writeRegister(0x09, buf, 4))
    {
      disable_alarm_irq();  // best effort: never arm a partial/old alarm
      return false;
    }
    // Clear AF for both arming and clearing while preserving a pending TF.
    std::uint8_t control2 = 0;
    if (!readRegister(0x01, &control2, 1)
     || !writeRegister8(0x01, (control2 & 0x11) | 0x04))
    {
      disable_alarm_irq();
      return false;
    }
    if (irq_enable
     && (!readRegister(0x01, &control2, 1)
      || !writeRegister8(0x01, (control2 & 0x11) | 0x0E)))
    {
      disable_alarm_irq();
      return false;
    }
    return true;
  }

  bool PCF8563_Class::getIRQstatus(void)
  {
    return _init && (0x0C & readRegister8(0x01));
  }

  bool PCF8563_Class::clearIRQ(void)
  {
    if (!_init) { return false; }
    return bitOff(0x01, 0x0C);
  }

  bool PCF8563_Class::disableIRQ(void)
  {
    if (!_init) { return false; }
    // disable alerm (bit7:1=disabled)
    static constexpr const std::uint8_t buf[4] = { 0x80, 0x80, 0x80, 0x80 };
    bool ok = writeRegister(0x09, buf, 4);

    // disable timer (bit7:0=disabled)
    ok = writeRegister8(0x0E, 0) && ok;

    // clear flag and INT enable bits
    ok = writeRegister8(0x01, 0x00) && ok;
    return ok;
  }
}
