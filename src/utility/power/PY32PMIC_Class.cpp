// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "PY32PMIC_Class.hpp"

#if __has_include(<esp_log.h>)
#include <esp_log.h>
#endif

#include <algorithm>

namespace m5
{
// PY32PMIC registers
  static constexpr const uint8_t PY32PMIC_REG_UID_L     = 0x00;
  static constexpr const uint8_t PY32PMIC_REG_UID_H     = 0x01;
  static constexpr const uint8_t PY32PMIC_REG_HW_REV    = 0x02;
  static constexpr const uint8_t PY32PMIC_REG_SW_REV    = 0x03;

  static constexpr const uint8_t PY32PMIC_REG_GPIO_MODE = 0x04;
  static constexpr const uint8_t PY32PMIC_REG_GPIO_OUT  = 0x05;
  static constexpr const uint8_t PY32PMIC_REG_GPIO_IN   = 0x06;

  static constexpr const uint8_t PY32PMIC_REG_PWR_CFG   = 0x1C;

  static constexpr const uint8_t PY32PMIC_REG_IRQ_Status3 = 0x23;
  static constexpr const uint8_t PY32PMIC_REG_SYS_CMD   = 0x32;

  bool PY32PMIC_Class::begin(void)
  {
    if (!_init) {
      std::uint32_t val = 0;
      auto res = readRegister(0x00, (uint8_t*)&val, 4);
      if (res)
      {
        writeRegister8(0x15, 0xA5); // WDT_KEY set

writeRegister8(0x10, 0x01); // WDT_KEY set

      // res = (val == PY32PMIC_CHIP_ID);
        // if (res) {
        //   // disable wdt
        //   if (readRegister(PY32PMIC_REG_CHR_TMR, &val, 1))
        //   {
        //     val = val & 0x1F;
        //     writeRegister8(PY32PMIC_REG_CHR_TMR, val);
        //   }
        // }
      }
      // writeRegister8(0x20,0b01100001); // BTN_CFG / LONG:0b11==4sec / SINGLE_RESET_DIS:1==en
      // writeRegister8(0x20,0b00011000); // BTN_CFG / LONG:0b11==4sec / SINGLE_RESET_DIS:1==en
      writeRegister8(0x20,0b00000101); // BTN_CFG / LONG:0b11==4sec / SINGLE_RESET_DIS:1==en
      // writeRegister8(0x31,1); // BTN_CFG_2 DOUBLE_POWER_OFF_DIS = 1
      writeRegister8(0x31,0); // BTN_CFG_2 DOUBLE_POWER_OFF_DIS = 0
      // writeRegister8(0x2F,0b00000111);
      // writeRegister8(0xA2,0b00000111);
      // writeRegister8(0x2F,0xFF); // WakeSrc
      // writeRegister8(0xA2,0xFF); // IRQ Status3_MASK
      writeRegister8(0x2F,0); // WakeSrc
      // writeRegister8(0x2F,1); // WakeSrc
      _init = res;
    }
    return _init;
  }

  bool PY32PMIC_Class::setExtOutput(bool enable)
  {
    if (!_init) { return false; }
    if (enable) {
      return bitOn(PY32PMIC_REG_PWR_CFG, 1 << 3); // set bit 3 to enable external output
    } else {
      return bitOff(PY32PMIC_REG_PWR_CFG, 1 << 3); // clear bit 3 to disable external output
    }
  }

  bool PY32PMIC_Class::setBatteryCharge(bool enable)
  {
    if (!_init) { return false; }
    if (enable) {
      return bitOn(PY32PMIC_REG_PWR_CFG, 1 << 0); // set bit 0 to enable battery charge
    } else {
      return bitOff(PY32PMIC_REG_PWR_CFG, 1 << 0); // clear bit 0 to disable battery charge
    }
  }

  bool PY32PMIC_Class::setChargeCurrent(std::uint16_t max_mA)
  {
    return false;
    // if (!_init) return false;
    // int value = max_mA / 8;     // Convert mA to register value (8mA per step)
    // if (value > 0) { value -= 1;  // 0 = 8mA, 63 = 512mA
    //   if (value >= 64) value = 63; // max value is 512mA (8 + 63*8)
    // }
    // return writeRegister8(PY32PMIC_REG_CHR_CUR, value);
  }

  bool PY32PMIC_Class::setChargeVoltage(std::uint16_t max_mV)
  {
    return false;
    // if (!_init) return false;
    // int value = (max_mV - 3600) / 15;     // Convert mV to register value (15mV per step)
    // if (value > 0) { value -= 1;  // 0 = 3600mV, 63 = 4545mV
    //   if (value >= 64) value = 63; // max value is 4545mV (3600 + 63*15)
    // }
    // uint8_t reg_value = readRegister8(PY32PMIC_REG_CHR_VOL);
    // reg_value &= 0xC0;
    // return writeRegister8(PY32PMIC_REG_CHR_VOL, reg_value | value);
  }

  std::uint16_t PY32PMIC_Class::getChargeCurrent(void)
  {
    return 0; // Return 0 if read failed
  }

  std::uint16_t PY32PMIC_Class::getChargeVoltage(void)
  {
    return 0; // Return 0 if read failed
  }

  bool PY32PMIC_Class::isCharging(void)
  {
    return false;
  }

  uint8_t PY32PMIC_Class::getPekPress(void)
  {
    if (!_init) return 0;
    uint8_t irq3 = 0;
    if (readRegister(PY32PMIC_REG_IRQ_Status3, &irq3, 1))
    {
      if (irq3) {
        // Clear the IRQ status after reading
        if (irq3 & 0x01) {
          writeRegister8(PY32PMIC_REG_IRQ_Status3, 0);
        }
// printf("PY32PMIC:IRQ3: %02X\n", irq3);
// fflush(stdout);
      }
    }
    return (irq3&1) ? 2 : 0;
  }

  bool PY32PMIC_Class::powerOff(void)
  {
    if (_init) {
      return writeRegister8(PY32PMIC_REG_SYS_CMD, 0xA1); // SYS_CMD / POWER_OFF
    }
    return false;
  }

} // namespace m5