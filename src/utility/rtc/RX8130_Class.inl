// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_IMPLEMENTATION
#error "RX8130_Class.inl is part of M5Unified.cpp and is not meant to be included on its own"
#endif

#include "RX8130_Class.hpp"

#include <stdlib.h>

namespace m5
{
  // flag_clear: W0C mask written to 0x1D once the timer is confirmed stopped
  // (0xAF clears TF only, 0xA7 clears TF and AF).
  // ctl_bits: the 0x1E bits to clear together with TE (0x10 = TIE, 0x18 = TIE and AIE).
  static bool stop_rx8130_timer(RX8130_Class* rtc, std::uint8_t flag_clear, std::uint8_t ctl_bits = 0x10)
  {
    for (int retry = 0; retry < 3; ++retry)
    {
      std::uint8_t ext = 0, ctl = 0;
      bool te_off = rtc->bitOff(0x1C, 0x10);
      bool tie_off = rtc->bitOff(0x1E, ctl_bits);
      bool ext_read = rtc->readRegister(0x1C, &ext, 1);
      bool ctl_read = rtc->readRegister(0x1E, &ctl, 1);
      if (te_off && tie_off && ext_read && ctl_read
       && !(ext & 0x10) && !(ctl & ctl_bits)
       && rtc->writeRegister8(0x1D, flag_clear))  // W0C: only the selected flags are cleared
      {
        return true;
      }
    }
    return false;
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
    if (!isEnabled() || !validateDateTime(date, time)) { return false; }
    // The year register holds two digits only (2000-2099).
    if (date && (date->year < 2000 || date->year > 2099)) { return false; }
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

  bool RX8130_Class::setTimerIRQ(std::uint32_t msec, std::uint32_t* applied_msec)
  {
    if (!isEnabled()) { return false; }
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
    const clk_t* fallback = nullptr;
    std::uint32_t fallback_cycle = 0;
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
        // Remember the first clock that can hold the request at all: it is used instead of
        // 4096 Hz when no clock meets the accuracy rule, because a 4096 Hz event (122us
        // /IRQ pulse) is lost behind the M5PM1 relay while a rounded period is still a wake-up.
        if (fallback == nullptr && cnt >= 1) { fallback = &c; fallback_cycle = cnt; }
      }
      if (sel == &clks[NCLK - 1] && fallback != nullptr && msec >= 250) {
        sel = fallback; cycle = fallback_cycle;
      }
      if (sel == nullptr) { return false; }  // unreachable (1/3600Hz covers all of uint32); fail safe = stay stopped
    }

    // Sequence per datasheet Figure 48: TE=0 (+TSEL) -> clear TF -> TIE -> preset -> TE=1 last,
    // so the first event cannot precede TIE. On any I2C failure the timer is stopped (verified by
    // read-back where the bus allows it) and false is returned; if even the stop fails the
    // hardware state is unknown.
    // 0x1D flags are write-0-to-clear (writing 1 is ignored, VBFF is read-only), so TF is cleared
    // with a single write that leaves the other flags untouched (a read-modify-write would drop
    // a flag raised in between).
    static constexpr std::uint8_t FLAG_CLEAR_TF = 0xAF;
    std::uint8_t reg0x1C = 0;
    if (cycle == 0) {
      if (!stop_rx8130_timer(this, FLAG_CLEAR_TF)) { return false; }
      if (applied_msec) { *applied_msec = 0; }
      return true;
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
      stop_rx8130_timer(this, FLAG_CLEAR_TF);
      return false;
    }
    if (applied_msec) {
      // Actual period rounded to the nearest ms (cycle * mul_ms fits by max_cnt); never 0 while running.
      std::uint32_t result = (cycle * sel->mul_ms + (sel->div >> 1)) / sel->div;
      *applied_msec = result ? result : 1;
    }
    return true;
  }

  bool RX8130_Class::setAlarmIRQ(const rtc_date_t* date, const rtc_time_t* time)
  {
    if (!validateAlarmFields(date, time) || !isEnabled()) { return false; }
    std::uint8_t buf[4] = { 0x80, 0x80, 0x80, 0x00 };

    bool irq_enable = false;
    int flg_wada = -1;
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
      }
    }

    // Keep AIE off until WADA and all alarm registers are committed.
    // The initial disable is retried: if it never succeeds the old alarm stays armed.
    bool disabled = false;
    for (int retry = 0; retry < 3 && !disabled; ++retry) { disabled = bitOff(0x1E, 0x08); }
    if (!disabled) { return false; }
    if (flg_wada >= 0
     && !(flg_wada ? bitOn(0x1C, 0x08) : bitOff(0x1C, 0x08)))
    {
      bitOff(0x1E, 0x08);
      return false;
    }
    if (!writeRegister(0x17, buf, 3))
    {
      bitOff(0x1E, 0x08);
      return false;
    }
    // 0x1D is W0C: clear AF for both arming and clearing while retaining TF.
    if (!writeRegister8(0x1D, 0xB7))
    {
      bitOff(0x1E, 0x08);
      return false;
    }
    if (irq_enable)
    {
      if (!bitOn(0x1E, 0x08))
      {
        bitOff(0x1E, 0x08);
        return false;
      }
    }
    return true;
  }

  bool RX8130_Class::getIRQstatus(void)
  {
    if (!isEnabled()) { return false; }
    // 0x10: Timer IRQ
    // 0x08: Alarm IRQ
    return readRegister8(0x1D) & 0x18;
  }

  bool RX8130_Class::clearIRQ(void)
  {
    if (!isEnabled()) { return false; }
    return writeRegister8(0x1D, 0xA7);  // W0C: clear TF and AF only
  }

  bool RX8130_Class::disableIRQ(void)
  {
    if (!isEnabled()) { return false; }
    // TIE, AIE and TE are cleared together with read-back and retry; W0C clears TF and AF.
    return stop_rx8130_timer(this, 0xA7, 0x18);
  }

  bool RX8130_Class::getVoltLow(void)
  {
    if (!isEnabled()) { return 0; }
    // 0x80: VBLF
    return readRegister8(0x1D) & 0x80;
  }
}
