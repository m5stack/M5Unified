// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "RX8130_Class.hpp"

#include <stdlib.h>

namespace m5
{
  static std::uint8_t bcd2ToByte(std::uint8_t value)
  {
    return ((value >> 4) * 10) + (value & 0x0F);
  }

  static std::uint8_t byteToBcd2(std::uint8_t value)
  {
    std::uint_fast8_t bcdhigh = value / 10;
    return (bcdhigh << 4) | (value - (bcdhigh * 10));
  }

  bool RX8130_Class::begin(I2C_Class* i2c)
  {
    if (i2c)
    {
      _i2c = i2c;
      i2c->begin();
    }

    bool res = true;
    res &= writeRegister8(0x30, 0x00);

    _init = res;
    return _init;
  }

  bool RX8130_Class::getDateTime(rtc_date_t* date, rtc_time_t* time) const
  {
    std::uint8_t buf[7] = { 0 };
    int start_reg = (time != nullptr) ? 0x10 : 0x13;
    int len = ((date != nullptr) ? 4 : 0)
            + ((time != nullptr) ? 3 : 0);
    if (!isEnabled() || len == 0 || !readRegister(start_reg, buf, len))
    {
      return false;
    }

    int idx = 0;
    if (time) {
      time->seconds = bcd2ToByte(buf[idx++] & 0x7f);
      time->minutes = bcd2ToByte(buf[idx++] & 0x7f);
      time->hours   = bcd2ToByte(buf[idx++] & 0x3f);
    }
    if (date) {
      date->weekDay = __builtin_ctz(buf[idx++]);
      date->date    = bcd2ToByte(buf[idx++] & 0x3f);
      date->month   = bcd2ToByte(buf[idx++] & 0x1f);
      date->year    = bcd2ToByte(buf[idx] & 0xff)
                    + ((0x80 & buf[idx - 1]) ? 1900 : 2000);
    }
    return true;
  }

  bool RX8130_Class::setDateTime(const rtc_date_t* date, const rtc_time_t* time)
  {
    std::uint8_t buf[7] = { 0 };

    int idx = 0;
    int reg_start = 0x13;
    if (time)
    {
      reg_start = 0x10;
      buf[idx++] = byteToBcd2(time->seconds);
      buf[idx++] = byteToBcd2(time->minutes);
      buf[idx++] = byteToBcd2(time->hours);
    }
    if (date)
    {
      buf[idx++] = (uint8_t)(1u << (7 & date->weekDay));
      buf[idx++] = byteToBcd2(date->date);
      buf[idx++] = (std::uint8_t)(byteToBcd2(date->month) + (date->year < 2000 ? 0x80 : 0));
      buf[idx++] = byteToBcd2(date->year % 100);
    }

    if (idx == 0) { return false; }
    return writeRegister(reg_start, buf, idx);
  }

  bool RX8130_Class::getVoltLow(void)
  {
    return false;
  }

  int RX8130_Class::setAlarmIRQ(int afterSeconds)
  {
    return false;
  }

  int RX8130_Class::setAlarmIRQ(const rtc_time_t &time)
  {
    return false;
  }

  int RX8130_Class::setAlarmIRQ(const rtc_date_t &date, const rtc_time_t &time)
  {
    return false;
  }

  bool RX8130_Class::getIRQstatus(void)
  {
    return false;
  }

  void RX8130_Class::clearIRQ(void)
  {
  }

  void RX8130_Class::disableIRQ(void)
  {
  }
}
