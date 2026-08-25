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

    // Decode into locals and validate before committing, so that a corrupted
    // register value (I2C glitch, uninitialized RTC) results in false instead
    // of a bogus date such as 45:00:80 or year 2165.
    int idx = 0;
    rtc_time_t t;
    if (time) {
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
    if (date) {
      // The weekday register holds a single bit for the current day; anything
      // else is invalid. (Also guards __builtin_ctz(0), which is undefined.)
      std::uint8_t wd = buf[idx++];
      std::uint8_t dd = buf[idx++] & 0x3f;
      std::uint8_t mo = buf[idx++] & 0x1f;
      std::uint8_t yy = buf[idx];
      if (wd == 0 || (wd & (wd - 1)) != 0
       || !isValidBcd(dd) || !isValidBcd(mo) || !isValidBcd(yy))
      {
        return false;
      }
      d.weekDay = __builtin_ctz(wd);
      d.date    = bcd2ToByte(dd);
      d.month   = bcd2ToByte(mo);
      d.year    = bcd2ToByte(yy) + 2000;
    }

    if (!validateDateTime(date ? &d : nullptr, time ? &t : nullptr))
    {
      return false;
    }
    if (time) { *time = t; }
    if (date) { *date = d; }
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
    // Source clocks in the order they are tried. period = mul_ms / div [ms].
    // max_ms  = 65535 * period, so msec <= max_ms keeps msec * div within uint32.
    // max_cnt = min(65535, 0xFFFFFFFF / mul_ms), so cnt * mul_ms (the period) stays within uint32.
    struct clk_t { std::uint32_t mul_ms; std::uint32_t div; std::uint32_t max_ms; std::uint16_t max_cnt; std::uint8_t tsel; };
    static constexpr clk_t clks[] = {
      {    1000, 64,   1023984,    65535, 0x01 }, // 64 Hz
      {    1000, 1,    65535000,   65535, 0x02 }, // 1 Hz
      {   60000, 1,    3932100000, 65535, 0x03 }, // 1/60 Hz
      { 3600000, 1,    0xFFFFFFFF, 1193,  0x04 }, // 1/3600 Hz
      {    1000, 4096, 15999,      65535, 0x00 }, // 4096 Hz (last resort: its /IRQ pulse is only 122us)
    };
    static constexpr std::size_t NCLK = sizeof(clks) / sizeof(clks[0]);
    // The /IRQ pulse auto-releases after 122us with the 4096Hz clock but 7.57ms with the others,
    // so take the finest non-4096Hz clock whose rounded count keeps the period error under 1/256
    // and has >= MIN_COUNT counts (the first countdown can be short by up to one source clock,
    // 1s for the 1/60Hz and 1/3600Hz clocks, so this bounds that to ~6% or less).
    static constexpr std::uint32_t MIN_COUNT = 16;

    std::uint32_t cycle = 0;
    const clk_t* sel = nullptr;
    if (msec != 0) {
      bool overflowed = false;  // a finer clock ran out of range: round up so the period never steps back
      for (std::size_t i = 0; i < NCLK; ++i) {
        const clk_t& c = clks[i];
        if (msec > c.max_ms) { overflowed = true; continue; }
        // Everything below is in units of msec * div: cnt counts of mul_ms each, err the remainder.
        std::uint32_t num = msec * c.div;
        std::uint32_t cnt = num / c.mul_ms;
        std::uint32_t err = num % c.mul_ms;
        if (overflowed ? (err != 0) : (err * 2 >= c.mul_ms)) { ++cnt; err = c.mul_ms - err; }
        if (cnt > c.max_cnt) { cnt = c.max_cnt; err = num - cnt * c.mul_ms; }
        // Accept when the error is within 1/256 (~0.39%) of the request (err < mul_ms, so no overflow).
        if (i + 1 == NCLK || (cnt >= MIN_COUNT && (err << 8) <= num)) {
          sel = &c; cycle = cnt; break;
        }
      }
      if (sel == nullptr) { return 0; }  // unreachable (1/3600Hz covers all of uint32); fail safe = stay stopped
    }

    // Sequence per datasheet Figure 48: TE=0 (+TSEL) -> clear TF -> TIE -> preset -> TE=1 last,
    // so the first event cannot precede TIE. On any I2C failure the timer is stopped (verified by
    // read-back where the bus allows it) and 0 is returned; the caller cannot tell that from a
    // requested stop, and if even the stop fails the hardware state is unknown.
    // 0x1D flags are write-0-to-clear (writing 1 is ignored, VBFF is read-only), so TF is cleared
    // with a single write that leaves the other flags untouched (a read-modify-write would drop
    // a flag raised in between).
    static constexpr std::uint8_t FLAG_CLEAR_TF = 0xAF;
    auto stop_timer = [this](void) -> bool {
      for (int retry = 0; retry < 3; ++retry) {
        std::uint8_t ext = 0, ctl = 0;
        if (bitOff(0x1C, 0x10) && bitOff(0x1E, 0x10)
         && readRegister(0x1C, &ext, 1) && readRegister(0x1E, &ctl, 1)
         && !(ext & 0x10) && !(ctl & 0x10)) { return true; }
      }
      return false;
    };
    std::uint8_t reg0x1C = 0;
    if (cycle == 0) {
      stop_timer();
      writeRegister8(0x1D, FLAG_CLEAR_TF);
      return 0;
    }
    bool ok = readRegister(0x1C, &reg0x1C, 1);
    if (ok) {
      reg0x1C = (reg0x1C & ~0x17) | sel->tsel;
      ok = writeRegister8(0x1C, reg0x1C)
        && writeRegister8(0x1D, FLAG_CLEAR_TF)
        && bitOn(0x1E, 0x10);
    }
    if (ok) {
      // While TE=0 the counter registers read back the preset, so verify the write took
      // (a corrupted preset was observed on a shared bus) and retry a few times.
      std::uint8_t regdata[2] = { (std::uint8_t)(cycle & 0xff), (std::uint8_t)((cycle >> 8) & 0xff) };
      ok = false;
      for (int retry = 0; retry < 3 && !ok; ++retry) {
        std::uint8_t verify[2] = { 0, 0 };
        ok = writeRegister(0x1A, regdata, 2)
          && readRegister(0x1A, verify, 2)
          && verify[0] == regdata[0] && verify[1] == regdata[1];
      }
    }
    if (ok) { ok = writeRegister8(0x1C, reg0x1C | 0x10); }
    if (!ok) {
      stop_timer();
      return 0;
    }
    // Actual period rounded to the nearest ms (cycle * mul_ms fits by max_cnt); never 0 while running.
    std::uint32_t result = (cycle * sel->mul_ms + (sel->div >> 1)) / sel->div;
    return result ? result : 1;
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
      writeRegister8(0x1D, 0xA7);  // W0C: clear TF and AF only
    }
  }

  void RX8130_Class::disableIRQ(void)
  {
    if (isEnabled()) {
      bitOff(0x1E, 0x18);
      writeRegister8(0x1D, 0xA7);  // W0C: clear TF and AF only
    }
  }

  bool RX8130_Class::getVoltLow(void)
  {
    if (!isEnabled()) { return 0; }
    // 0x80: VBLF
    return readRegister8(0x1D) & 0x80;
  }
}
