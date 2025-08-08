// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "AW32001_Class.hpp"

#if __has_include(<esp_log.h>)
#include <esp_log.h>
#endif

#include <algorithm>

namespace m5
{
// AW32001 registers
  static constexpr const uint8_t AW32001_REG_PWR_CFG = 0x01; // Power Configuration
  static constexpr const uint8_t AW32001_REG_CHR_CUR = 0x02; // Charging current
  static constexpr const uint8_t AW32001_REG_CHR_VOL = 0x04; // Charge voltage
  static constexpr const uint8_t AW32001_REG_CHR_TMR = 0x05; // Charge timer
  static constexpr const uint8_t AW32001_REG_SYS_STA = 0x08; // System status
  static constexpr const uint8_t AW32001_REG_CHIP_ID = 0x0A; // ChipID

  static constexpr const uint8_t AW32001_CHIP_ID = 0x49; // ChipID

  bool AW32001_Class::begin(void)
  {
    std::uint8_t val = 0;
    auto res = readRegister(0x0A, &val, 1);
    if (res)
    {
      res = (val == AW32001_CHIP_ID);
      if (res) {
        // disable wdt
        if (readRegister(AW32001_REG_CHR_TMR, &val, 1))
        {
          val = val & 0x1F;
          writeRegister8(AW32001_REG_CHR_TMR, val);
        }
      }
    }
    _init = res;
    return res;
  }

  bool AW32001_Class::setBatteryCharge(bool enable)
  {
    if (!_init) { return false; }
    if (enable) {
      return bitOff(AW32001_REG_PWR_CFG, 1 << 3); // clear bit 3 to enable battery charge
    } else {
      return bitOn(AW32001_REG_PWR_CFG, 1 << 3); // set bit 3 to disable battery charge
    }
  }

  bool AW32001_Class::setChargeCurrent(std::uint16_t max_mA)
  {
    if (!_init) return false;
    int value = max_mA / 8;     // Convert mA to register value (8mA per step)
    if (value > 0) { value -= 1;  // 0 = 8mA, 63 = 512mA
      if (value >= 64) value = 63; // max value is 512mA (8 + 63*8)
    }
    return writeRegister8(AW32001_REG_CHR_CUR, value);
  }

  bool AW32001_Class::setChargeVoltage(std::uint16_t max_mV)
  {
    if (!_init) return false;
    int value = (max_mV - 3600) / 15;     // Convert mV to register value (15mV per step)
    if (value > 0) { value -= 1;  // 0 = 3600mV, 63 = 4545mV
      if (value >= 64) value = 63; // max value is 4545mV (3600 + 63*15)
    }
    uint8_t reg_value = readRegister8(AW32001_REG_CHR_VOL);
    reg_value &= 0xC0;
    return writeRegister8(AW32001_REG_CHR_VOL, reg_value | value);
  }

  std::uint16_t AW32001_Class::getChargeCurrent(void)
  {
    uint8_t reg_value = 0;
    if (_init && readRegister(AW32001_REG_CHR_CUR, &reg_value, 1))
    {
      return (reg_value & 0x3F) * 8 + 8; // Convert register value back to mA
    }
    return 0; // Return 0 if read failed
  }

  std::uint16_t AW32001_Class::getChargeVoltage(void)
  {
    uint8_t reg_value = 0;
    if (_init && readRegister(AW32001_REG_CHR_VOL, &reg_value, 1))
    {
      return (reg_value & 0x3F) * 15 + 3600; // Convert register value back to mV
    }
    return 0; // Return 0 if read failed
  }

  AW32001_Class::ChargeStatus AW32001_Class::getChargeStatus(void)
  {
    uint8_t reg_value = 0;
    if (_init && readRegister(AW32001_REG_CHR_VOL, &reg_value, 1))
    {
      // Extract bits 4 and 3 for charge status
      reg_value = (reg_value >> 3) & 0b00000011; // Get bits 4 and 3
      switch (reg_value)
      {
        case 0b00: return CS_NOT_CHARGING; // Not charging
        case 0b01: return CS_PRE_CHARGE;   // Pre-charge
        case 0b10: return CS_CHARGE;       // Charging
        case 0b11: return CS_CHARGE_DONE;  // Charge done
        default: return CS_UNKNOWN;        // Unknown state
      }
    }
    return CS_UNKNOWN; // Return unknown if read failed
  }
}
