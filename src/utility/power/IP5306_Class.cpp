// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "IP5306_Class.hpp"

#if __has_include(<esp_log.h>)
#include <esp_log.h>
#endif

#include <algorithm>

namespace m5
{
  static constexpr std::uint8_t REG_SYS_CTL0 = 0x00;
  static constexpr std::uint8_t REG_SYS_CTL1 = 0x01;
  static constexpr std::uint8_t REG_SYS_CTL2 = 0x02;
  static constexpr std::uint8_t REG_READ0    = 0x70;
  static constexpr std::uint8_t REG_READ1    = 0x71;
  static constexpr std::uint8_t REG_READ2    = 0x72;
  static constexpr std::uint8_t REG_READ3    = 0x77;
  static constexpr std::uint8_t REG_READ4    = 0x78;
  static constexpr std::uint8_t REG_CHG_CTL0 = 0x20;
  static constexpr std::uint8_t REG_CHG_CTL1 = 0x21;
  static constexpr std::uint8_t REG_CHG_CTL2 = 0x22;
  static constexpr std::uint8_t REG_CHG_CTL3 = 0x23;
  static constexpr std::uint8_t REG_CHG_DIG_CTL0 = 0x24;

  static constexpr std::uint8_t BOOST_OUT_BIT = 0x02;

  bool IP5306_Class::begin(void)
  {
    std::uint8_t val = 0;
    writeRegister(0x06, &val, 1); // reg06h WLED flashlight disabled
    val = 2;
    _init = writeRegister(0x06, &val, 1);
    if (_init)
    {
#if defined (ESP_LOGV)
      // ESP_LOGV("IP5306", "found");
#endif
    }
    return _init;
  }

  std::int8_t IP5306_Class::getBatteryLevel(void)
  {
    std::uint8_t data;
    if (readRegister(REG_READ4, &data, 1)) {
      switch (data >> 4) {
        case 0x00: return 100;
        case 0x08: return 75;
        case 0x0C: return 50;
        case 0x0E: return 25;
        default:   return 0;
      }
    }
    return -1;
  }

  static constexpr std::uint8_t CHARGE_EN_BIT = 0x10;

  bool IP5306_Class::setBatteryCharge(bool enable)
  {
    std::uint8_t val = 0;
    if (!readRegister(REG_SYS_CTL0, &val, 1)) { return false; }
    return writeRegister8(REG_SYS_CTL0, enable ? (val | CHARGE_EN_BIT) : (val & (~CHARGE_EN_BIT)));
  }

  bool IP5306_Class::getBatteryCharge(bool* enabled)
  {
    std::uint8_t val = 0;
    if (enabled == nullptr || !readRegister(REG_SYS_CTL0, &val, 1)) { return false; }
    *enabled = (val & CHARGE_EN_BIT) != 0;
    return true;
  }

  bool IP5306_Class::readChargeActive(bool* active)
  {
    std::uint8_t val = 0;
    if (active == nullptr || !readRegister(REG_READ0, &val, 1)) { return false; }
    *active = (val & 0x08) != 0;
    return true;
  }

  bool IP5306_Class::readChargeFull(bool* full)
  { /// REG_READ0 and REG_READ1 sit at adjacent addresses but are read
    /// separately on purpose: the register document only ever shows a
    /// single byte read and nowhere states that the address auto-increments.
    std::uint8_t val = 0;
    if (full == nullptr || !readRegister(REG_READ1, &val, 1)) { return false; }
    *full = (val & 0x08) != 0;
    return true;
  }

  bool IP5306_Class::setChargeCurrent(std::uint16_t max_mA, std::uint16_t* applied_mA)
  { /// the register is a weighted sum with a 100mA step over a 50mA floor.
    std::uint16_t steps = (max_mA > 50) ? (max_mA - 50) / 100 : 0;
    if (steps > 31) { steps = 31; }

    std::uint8_t val = 0;
    if (!readRegister(REG_CHG_DIG_CTL0, &val, 1)) { return false; }
    if (!writeRegister8(REG_CHG_DIG_CTL0, (val & 0xE0) + steps)) { return false; }
    if (applied_mA) { *applied_mA = (std::uint16_t)(50 + steps * 100); }
    return true;
  }

  bool IP5306_Class::setChargeVoltage(std::uint16_t max_mV, std::uint16_t* applied_mV)
  { /// reg CHG_CTL2 selects the constant-voltage target plus the boost the
    /// datasheet recommends for each step (28mV at 4.2V, 14mV above). The
    /// cell sees the sum, so the steps are compared and reported as the
    /// effective values: the highest one not above the request, clamped up to
    /// the lowest when the request is under all of them.
    static constexpr std::uint16_t table[4] = { 4228, 4314, 4364, 4414 };
    static constexpr std::uint8_t regdata[4] =
      { 0x02 // 4.2v  + boost 28mV
      , 0x05 // 4.3v  + boost 14mV
      , 0x09 // 4.35v + boost 14mV
      , 0x0D // 4.4v  + boost 14mV
      };
    size_t i = 3;
    while (i && table[i] > max_mV) { --i; }
    if (!writeRegister8(REG_CHG_CTL2, regdata[i])) { return false; }
    if (applied_mV) { *applied_mV = table[i]; }
    return true;
  }

  bool IP5306_Class::isCharging(void)
  { /// This needs both of the flags the datasheet describes, not one of them:
    /// REG_READ0 bit3 tells charging from discharging, and REG_READ1 bit3
    /// tells whether the cell has already been filled. Only the first was
    /// read. It stays set once charging is enabled and a supply is present -
    /// the completed charge included - so a finished charge was reported as an
    /// ongoing one, as was a board running with no cell installed at all.
    ///
    /// The two sit at adjacent addresses but are read separately on purpose.
    /// The register document only ever shows a single-byte read and nowhere
    /// states that the address auto-increments, so reading both in one
    /// transaction would rest on behaviour that is not specified.
    ///
    /// Two limits worth knowing. The full flag has no defined reset value, so
    /// shortly after power-up it can read as full before the charger has
    /// settled, and nothing here can tell that from a real full charge. And a
    /// failed read is reported the same way as "not charging", because this
    /// return type has no room to say that the question could not be answered.
    std::uint8_t val = 0;
    if (!readRegister(REG_READ0, &val, 1)) { return false; }
    /// discharging: either charging is disabled or there is no supply
    if (!(val & 0x08)) { return false; }
    if (!readRegister(REG_READ1, &val, 1)) { return false; }
    /// already full, so nothing is going into the cell
    return !(val & 0x08);
  }

  bool IP5306_Class::setPowerBoostKeepOn(bool en) {
    std::uint8_t data;
    if (readRegister(REG_SYS_CTL0, &data, 1) == true)
    {
      data = en ? (data | BOOST_OUT_BIT) : (data & (~BOOST_OUT_BIT));
      return writeRegister(REG_SYS_CTL0, &data, 1);
    }
    return false;
  }
}
