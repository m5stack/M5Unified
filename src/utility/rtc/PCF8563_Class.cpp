// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "PCF8563_Class.hpp"

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

    int idx = 0;
    if (time)
    {
      time->seconds = bcd2ToByte(buf[idx++] & 0x7f);
      time->minutes = bcd2ToByte(buf[idx++] & 0x7f);
      time->hours   = bcd2ToByte(buf[idx++] & 0x3f);
    }

    if (date)
    {
      date->date    = bcd2ToByte(buf[idx++] & 0x3f);
      date->weekDay = bcd2ToByte(buf[idx++] & 0x07);
      date->month   = bcd2ToByte(buf[idx++] & 0x1f);
      date->year    = bcd2ToByte(buf[idx] & 0xff)
                    + ((0x80 & buf[idx - 1]) ? 1900 : 2000);
    }
    return true;
  }

  bool PCF8563_Class::setDateTime(const rtc_date_t* date, const rtc_time_t* time)
  {
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

  std::uint32_t PCF8563_Class::setTimerIRQ(std::uint32_t msec)
  {
    std::uint8_t reg_value = readRegister8(0x01) & ~0x0C;

    std::uint32_t afterSeconds = (msec + 500) / 1000;
    if (afterSeconds <= 0)
    { // disable timer
      writeRegister8(0x01, reg_value & ~0x01);
      writeRegister8(0x0E, 0x03);
      return 0;
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

    writeRegister8(0x0E, type_value);
    writeRegister8(0x0F, afterSeconds);

    writeRegister8(0x01, (reg_value | 0x01) & ~0x80);
    return afterSeconds * div * 1000;
  }

  int PCF8563_Class::setAlarmIRQ(const rtc_date_t *date, const rtc_time_t *time)
  {
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

    writeRegister(0x09, buf, 4);

    if (irq_enable)
    {
      bitOn(0x01, 0x02);
    }
    else
    {
      bitOff(0x01, 0x02);
    }

    return irq_enable;
  }

  bool PCF8563_Class::getIRQstatus(void)
  {
    return _init && (0x0C & readRegister8(0x01));
  }

  void PCF8563_Class::clearIRQ(void)
  {
    if (!_init) { return; }
    bitOff(0x01, 0x0C);
  }

  void PCF8563_Class::disableIRQ(void)
  {
    if (!_init) { return; }
    // disable alerm (bit7:1=disabled)
    static constexpr const std::uint8_t buf[4] = { 0x80, 0x80, 0x80, 0x80 };
    writeRegister(0x09, buf, 4);

    // disable timer (bit7:0=disabled)
    writeRegister8(0x0E, 0);

    // clear flag and INT enable bits
    writeRegister8(0x01, 0x00);
  }
}
