// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "RTC_PowerHub_Class.hpp"
#include <algorithm>
#include <stdlib.h>

namespace m5
{
  static std::uint8_t byteToBcd2(std::uint8_t value)
  {
    std::uint_fast8_t bcdhigh = value / 10;
    return (bcdhigh << 4) | (value - (bcdhigh * 10));
  }

  bool RTC_PowerHub_Class::begin(I2C_Class* i2c)
  {
    if (i2c)
    {
      _i2c = i2c;
      i2c->begin();
    }
    _init = 1;
    return _init;
  }

  bool RTC_PowerHub_Class::getVoltLow(void)
  {
    return readRegister8(0x02) & 0x80; // RTCC_VLSEC_MASK
  }

  bool RTC_PowerHub_Class::getDateTime(rtc_date_t* date, rtc_time_t* time) const
  {
    std::uint8_t buf[7] = { 0 };
    int start_reg = (time != nullptr) ? 0xC0 : 0xC3;
    int len = (time != nullptr) ? 7 : 4;
    if (!isEnabled() || !readRegister(start_reg, buf, len))
    {
        return false;
    }

    int idx = 0;
    if (time)
    {
      time->seconds = buf[idx++] & 0x3f;
      time->minutes = buf[idx++] & 0x3f;
      time->hours   = buf[idx++] & 0x1f;
    }

    if (date)
    {
      date->date    = buf[idx++] & 0x1f;
      date->month   = buf[idx++] & 0x0f;
      date->year    = (buf[idx++] & 0x7f) + 2000;
      date->weekDay = __builtin_ctz(byteToBcd2(buf[idx++]));
    }
    return true;
  }

  bool RTC_PowerHub_Class::setDateTime(const rtc_date_t* date, const rtc_time_t* time)
  {
    std::uint8_t buf[7] = { 0 };
    int idx = 0;
    int reg_start = 0xC3;

    if (time)
    {
      reg_start = 0xC0;
      buf[idx++] = time->seconds & 0x3f;
      buf[idx++] = time->minutes & 0x3f;
      buf[idx++] = time->hours   & 0x1f;
    }

    if (date)
    {
      buf[idx++] = date->date  & 0x1f;
      buf[idx++] = date->month & 0x0f;
      buf[idx++] = (date->year >= 2000) ? ((date->year - 2000) & 0x7f) : (date->year & 0x7f);
      buf[idx++] = (uint8_t)(1u << (7 & date->weekDay));
    }

    if (idx == 0) { return false; }
    return writeRegister(reg_start, buf, idx);
  }

  std::uint32_t RTC_PowerHub_Class::setTimerIRQ(std::uint32_t timer_msec)
  {
    return false;
  }


  int RTC_PowerHub_Class::setAlarmIRQ(const rtc_date_t *date, const rtc_time_t *time)
  {

    std::uint8_t buf[3];
    bool irq_enable = false;

    if (time) {
      if (time->minutes >= 0)
      {
        irq_enable = true;
        buf[0] = time->minutes & 0x3f;
      }

      if (time->hours >= 0)
      {
        irq_enable = true;
        buf[1] = time->hours & 0x1f;

      }
    }
    if (date->date >= 0)
    {
      irq_enable = true;
      buf[2] = date->date & 0x1f;
    }

    writeRegister(0xD0, buf, 3);

    if (irq_enable)
    {
      bitOn(0xD3, 0);
    } else {
      bitOff(0xD3, 0);
    }

    return irq_enable;
  }

  bool RTC_PowerHub_Class::getIRQstatus(void)
  {
    return false;
  }

  void RTC_PowerHub_Class::clearIRQ(void)
  {
    if (!_init) { return; }
    static constexpr const std::uint8_t buf[4] = {};
    writeRegister(0xD0, buf, sizeof(buf));
  }

  void RTC_PowerHub_Class::disableIRQ(void)
  {
    if (!_init) { return; }
    bitOff(0xD3, 0); // disable alarm
  }
}
