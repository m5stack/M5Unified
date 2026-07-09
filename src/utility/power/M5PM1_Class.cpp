// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "M5PM1_Class.hpp"

#if __has_include(<esp_log.h>)
#include <esp_log.h>
#endif

#include <algorithm>

namespace m5
{
// M5PM1 registers
  static constexpr const uint8_t M5PM1_REG_DEVICE_ID   = 0x00;
  static constexpr const uint8_t M5PM1_REG_PWR_SRC     = 0x04;
  static constexpr const uint8_t M5PM1_REG_PWR_CFG     = 0x06;
  static constexpr const uint8_t M5PM1_REG_I2C_CFG     = 0x09;
  static constexpr const uint8_t M5PM1_REG_WDT_CNT     = 0x0A;
  static constexpr const uint8_t M5PM1_REG_SYS_CMD     = 0x0C;
  static constexpr const uint8_t M5PM1_REG_GPIO_MODE   = 0x10;
  static constexpr const uint8_t M5PM1_REG_GPIO_OUT    = 0x11;
  static constexpr const uint8_t M5PM1_REG_GPIO_IN     = 0x12;
  static constexpr const uint8_t M5PM1_REG_GPIO_DRV    = 0x13;
  static constexpr const uint8_t M5PM1_REG_GPIO_FUNC0  = 0x16;
  static constexpr const uint8_t M5PM1_REG_GPIO_FUNC1  = 0x17;
  static constexpr const uint8_t M5PM1_REG_VBAT_L      = 0x22;
  static constexpr const uint8_t M5PM1_REG_VIN_L       = 0x24;
  static constexpr const uint8_t M5PM1_REG_5VOUT_L     = 0x26;
  static constexpr const uint8_t M5PM1_REG_IRQ_STATUS3 = 0x42;

  static constexpr const uint8_t M5PM1_PWR_CFG_CHG_EN   = 1 << 0;
  static constexpr const uint8_t M5PM1_PWR_CFG_LDO_EN   = 1 << 2;
  static constexpr const uint8_t M5PM1_PWR_CFG_BOOST_EN = 1 << 3;
  static constexpr const uint8_t M5PM1_SYS_CMD_SHUTDOWN = 0xA1;

  static constexpr bool is_valid_gpio(M5PM1_Class::gpio_t pin)
  {
    return static_cast<std::uint8_t>(pin) < 5;
  }

  static constexpr std::uint8_t gpio_num(M5PM1_Class::gpio_t pin)
  {
    return static_cast<std::uint8_t>(pin);
  }

  bool M5PM1_Class::begin(void)
  {
    if (!_init) {
      std::uint32_t val = 0;
      auto res = readRegister(M5PM1_REG_DEVICE_ID, (uint8_t*)&val, 4);
      if (res)
      {
        writeRegister8(M5PM1_REG_I2C_CFG, 0x00); // disable I2C idle sleep
        writeRegister8(M5PM1_REG_WDT_CNT, 0x00); // disable watchdog
      }
      _init = res;
    }
    return _init;
  }

  bool M5PM1_Class::setExtOutput(bool enable)
  {
    return enable ? bitOn(M5PM1_REG_PWR_CFG, M5PM1_PWR_CFG_BOOST_EN)
                  : bitOff(M5PM1_REG_PWR_CFG, M5PM1_PWR_CFG_BOOST_EN);
  }

  bool M5PM1_Class::getExtOutput(void)
  {
    if (!_init) { return false; }
    return readRegister8(M5PM1_REG_PWR_CFG) & M5PM1_PWR_CFG_BOOST_EN;
  }

  bool M5PM1_Class::setLDOOutput(bool enable)
  {
    return enable ? bitOn(M5PM1_REG_PWR_CFG, M5PM1_PWR_CFG_LDO_EN)
                  : bitOff(M5PM1_REG_PWR_CFG, M5PM1_PWR_CFG_LDO_EN);
  }

  M5PM1_Class::pwr_src_t M5PM1_Class::getPowerSource(void)
  {
    if (!_init) { return unknown; }
    auto src = readRegister8(M5PM1_REG_PWR_SRC) & 0x07;
    return src <= static_cast<std::uint8_t>(battery) ? static_cast<pwr_src_t>(src) : unknown;
  }

  bool M5PM1_Class::setGPIOFunction(gpio_t pin, gpio_function_t function)
  {
    if (!is_valid_gpio(pin)) { return false; }
    auto num = gpio_num(pin);
    auto reg = num < 4 ? M5PM1_REG_GPIO_FUNC0 : M5PM1_REG_GPIO_FUNC1;
    auto shift = static_cast<std::uint8_t>((num < 4 ? num : num - 4) * 2);
    std::uint8_t mask = 0x03 << shift;
    std::uint8_t reg_val = readRegister8(reg);
    reg_val = (reg_val & ~mask) | (static_cast<std::uint8_t>(function) << shift);
    return writeRegister8(reg, reg_val);
  }

  bool M5PM1_Class::setGPIOMode(gpio_t pin, gpio_mode_t mode)
  {
    if (!is_valid_gpio(pin)) { return false; }
    auto mask = 1 << gpio_num(pin);
    return mode == output ? bitOn(M5PM1_REG_GPIO_MODE, mask)
                          : bitOff(M5PM1_REG_GPIO_MODE, mask);
  }

  bool M5PM1_Class::setGPIODrive(gpio_t pin, gpio_drive_t drive)
  {
    if (!is_valid_gpio(pin)) { return false; }
    auto mask = 1 << gpio_num(pin);
    return drive == open_drain ? bitOn(M5PM1_REG_GPIO_DRV, mask)
                               : bitOff(M5PM1_REG_GPIO_DRV, mask);
  }

  bool M5PM1_Class::setGPIOOutput(gpio_t pin, bool high)
  {
    if (!is_valid_gpio(pin)) { return false; }
    auto mask = 1 << gpio_num(pin);
    return high ? bitOn(M5PM1_REG_GPIO_OUT, mask) : bitOff(M5PM1_REG_GPIO_OUT, mask);
  }

  bool M5PM1_Class::getGPIOInput(gpio_t pin)
  {
    if (!_init || !is_valid_gpio(pin)) { return false; }
    return readRegister8(M5PM1_REG_GPIO_IN) & (1 << gpio_num(pin));
  }

  bool M5PM1_Class::getGPIOOutputLatch(gpio_t pin)
  {
    if (!_init || !is_valid_gpio(pin)) { return false; }
    return readRegister8(M5PM1_REG_GPIO_OUT) & (1 << gpio_num(pin));
  }

  bool M5PM1_Class::setBatteryCharge(bool enable)
  {
    return enable ? bitOn(M5PM1_REG_PWR_CFG, M5PM1_PWR_CFG_CHG_EN)
                  : bitOff(M5PM1_REG_PWR_CFG, M5PM1_PWR_CFG_CHG_EN);
  }

  bool M5PM1_Class::setChargeCurrent(std::uint16_t max_mA)
  {
    return false;
    // if (!_init) return false;
    // int value = max_mA / 8;     // Convert mA to register value (8mA per step)
    // if (value > 0) { value -= 1;  // 0 = 8mA, 63 = 512mA
    //   if (value >= 64) value = 63; // max value is 512mA (8 + 63*8)
    // }
    // return writeRegister8(M5PM1_REG_CHR_CUR, value);
  }

  bool M5PM1_Class::setChargeVoltage(std::uint16_t max_mV)
  {
    return false;
    // if (!_init) return false;
    // int value = (max_mV - 3600) / 15;     // Convert mV to register value (15mV per step)
    // if (value > 0) { value -= 1;  // 0 = 3600mV, 63 = 4545mV
    //   if (value >= 64) value = 63; // max value is 4545mV (3600 + 63*15)
    // }
    // uint8_t reg_value = readRegister8(M5PM1_REG_CHR_VOL);
    // reg_value &= 0xC0;
    // return writeRegister8(M5PM1_REG_CHR_VOL, reg_value | value);
  }

  std::uint16_t M5PM1_Class::getChargeCurrent(void)
  {
    return 0; // Return 0 if read failed
  }

  std::uint16_t M5PM1_Class::getChargeVoltage(void)
  {
    return 0; // Return 0 if read failed
  }

  bool M5PM1_Class::isCharging(void)
  {
    return false;
  }

  uint8_t M5PM1_Class::getPekPress(void)
  {
    if (!_init) return 0;
    uint8_t irq3 = 0;
    if (readRegister(M5PM1_REG_IRQ_STATUS3, &irq3, 1))
    {
      if (irq3 & ((1 << 0) | (1 << 2))) {
        writeRegister8(M5PM1_REG_IRQ_STATUS3, 0);
        return 2;
      }
    }
    return 0;
  }

  std::uint16_t M5PM1_Class::getVBUSVoltage(void)
  {
    if (!_init) { return 0; }
    std::uint8_t buf[2] = {};
    return readRegister(M5PM1_REG_VIN_L, buf, sizeof(buf)) ? (buf[1] << 8) | buf[0] : 0;
  }

  std::uint16_t M5PM1_Class::getBatteryVoltage(void)
  {
    if (!_init) { return 0; }
    std::uint8_t buf[2] = {};
    return readRegister(M5PM1_REG_VBAT_L, buf, sizeof(buf)) ? (buf[1] << 8) | buf[0] : 0;
  }

  std::uint16_t M5PM1_Class::get5VoutVoltage(void)
  {
    if (!_init) { return 0; }
    std::uint8_t buf[2] = {};
    return readRegister(M5PM1_REG_5VOUT_L, buf, sizeof(buf)) ? (buf[1] << 8) | buf[0] : 0;
  }

  bool M5PM1_Class::powerOff(void)
  {
    return _init && writeRegister8(M5PM1_REG_SYS_CMD, M5PM1_SYS_CMD_SHUTDOWN);
  }

} // namespace m5
