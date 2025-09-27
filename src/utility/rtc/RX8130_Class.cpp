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

    bool res = bitOn(0x1F, 0x30);
    res &= writeRegister8(0x30, 0x00);
    res &= writeRegister8(0x1E, 0x00);

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
      date->year    = bcd2ToByte(buf[idx] & 0xff) + 2000;
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
      buf[idx++] = byteToBcd2(date->month);
      buf[idx++] = byteToBcd2(date->year % 100);
    }

    if (!isEnabled() || idx == 0) { return false; }
    return writeRegister(reg_start, buf, idx);
  }

  std::uint32_t RX8130_Class::setTimerIRQ(std::uint32_t msec)
  {
    // タイマー周期の除数
    uint32_t div = 1;
    // タイマー周期の乗数
    uint32_t mul = 1;

    uint8_t tsel_bits = 0;
    if (msec < 65536 * 1000 / 4096) { // 約16秒
      tsel_bits = 0x00;
      div = 4096;
    } else if (msec < 65536 * 1000 / 64) { // 約1024秒(約17分)
      tsel_bits = 0x01;
      div = 64;
    } else if (msec < 65536 * 1000) { // 約65535秒(約18時間)
      tsel_bits = 0x02;
    } else if (msec < 65536 * 60) { // 約3,932,160秒(約45日)
      mul = 60;
      tsel_bits = 0x03;
    } else { // msec < 65536*3600 // 約39,321,600秒(約1年3ヶ月)
      mul = 3600;
      tsel_bits = 0x04;
    }

    std::uint32_t result = 0;
    std::uint8_t regdata[3];
    if (readRegister(0x1A, regdata, 3)) {
      mul *= 1000;
      uint32_t cycle = (msec * div + (mul >> 1)) / mul;
      if (cycle > 65535) { cycle = 65535; }
      result = cycle * mul / div;

      regdata[0] = cycle & 0xff;
      regdata[1] = (cycle >> 8) & 0xff;
      if (cycle > 0) {
        // Clear timer select bits & TE flag
        std::uint8_t reg0x1C = regdata[2] & ~0x17;
        // Set timer select bits & TE flag
        reg0x1C |= 0x10 | tsel_bits;
        regdata[2] = reg0x1C;
        bitOn(0x1E, 0x10);
      } else {
        // Clear TE flag
        regdata[2] &= ~0x10;
        bitOff(0x1E, 0x10);
      }
      writeRegister(0x1A, regdata, 3);
    }

    return result;
  }

  int RX8130_Class::setAlarmIRQ(const rtc_date_t* date, const rtc_time_t* time)
  {
    if (!isEnabled()) { return 0; }
    std::uint8_t buf[4] = { 0x80, 0x80, 0x80, 0x00 };

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
      // 0 Sets WEEK as target of alarm function
      // 1 Sets DAY as target of alarm function
      int flg_wada = -1;
      if (date->date >= 0)
      {
        flg_wada = 1;
        buf[2] = byteToBcd2(date->date) & 0x3f;
      }
      else if (date->weekDay >= 0)
      {
        flg_wada = 0;
        buf[2] = 1u << (date->weekDay & 0x07);
      }
      if (flg_wada >= 0)
      {
        irq_enable = true;
        if (flg_wada)
        { // week alarm / day alarm selector
          bitOn(0x1C, 0x08);
        } else {
          bitOff(0x1C, 0x08);
        }
      }
    }

    // MIN_ALARM_REG 0x17
    writeRegister(0x17, buf, 3);

    if (irq_enable)
    {
      bitOn(0x1E, 0x08);
    }
    else
    {
      bitOff(0x1E, 0x08);
    }

    return irq_enable;
  }

  bool RX8130_Class::getIRQstatus(void)
  {
    if (!isEnabled()) { return 0; }
    // 0x10: Timer IRQ
    // 0x08: Alarm IRQ
    return readRegister8(0x1D) & 0x18;
  }

  void RX8130_Class::clearIRQ(void)
  {
    if (isEnabled()) {
      bitOff(0x1D, 0x18);
    }
  }

  void RX8130_Class::disableIRQ(void)
  {
    if (isEnabled()) {
      bitOff(0x1E, 0x18);
    }
  }

  bool RX8130_Class::getVoltLow(void)
  {
    if (!isEnabled()) { return 0; }
    // 0x80: VBLF
    return readRegister8(0x1D) & 0x80;
  }
}
