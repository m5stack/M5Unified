// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "M5IOE1_Class.hpp"

namespace m5
{
  namespace
  {
    static constexpr std::uint8_t M5IOE1_REG_UID_L       = 0x00;
    static constexpr std::uint8_t M5IOE1_REG_GPIO_MODE_L = 0x03;
    static constexpr std::uint8_t M5IOE1_REG_GPIO_OUT_L  = 0x05;
    static constexpr std::uint8_t M5IOE1_REG_GPIO_IN_L   = 0x07;
    static constexpr std::uint8_t M5IOE1_REG_GPIO_PU_L   = 0x09;
    static constexpr std::uint8_t M5IOE1_REG_GPIO_PD_L   = 0x0B;
    static constexpr std::uint8_t M5IOE1_REG_GPIO_IE_L   = 0x0D;
    static constexpr std::uint8_t M5IOE1_REG_GPIO_IS_L   = 0x11;
    static constexpr std::uint8_t M5IOE1_REG_GPIO_DRV_L  = 0x13;
    static constexpr std::uint8_t M5IOE1_REG_PWM1_DUTY_L = 0x1B;
    static constexpr std::uint8_t M5IOE1_REG_I2C_CFG     = 0x23;
    static constexpr std::uint8_t M5IOE1_REG_PWM_FREQ_L  = 0x25;
    static constexpr std::uint8_t M5IOE1_PWM_POLARITY    = 1 << 6;
    static constexpr std::uint8_t M5IOE1_PWM_ENABLE      = 1 << 7;
    static constexpr std::uint32_t M5IOE1_I2C_FREQ_100K  = 100000;
  }

  bool M5IOE1_Class::begin()
  {
    if (!_init) {
      _freq = M5IOE1_I2C_FREQ_100K;
      std::uint8_t uid[2] = {};
      auto res = readRegister(M5IOE1_REG_UID_L, uid, sizeof(uid));
      if (res) {
        // Disable I2C idle sleep and keep the IOE1 bus at its default 100 kHz speed.
        writeRegister8(M5IOE1_REG_I2C_CFG, 0x00);
      }
      _init = res;
    }
    return _init;
  }

  void M5IOE1_Class::setDirection(uint8_t pin, bool direction)
  {
    if (!_isValidPin(pin)) { return; }
    // false=input, true=output. MODE bit set means output.
    const auto reg = _regForPin(M5IOE1_REG_GPIO_MODE_L, pin);
    const auto bit = _bitForPin(pin);
    direction ? bitOn(reg, bit) : bitOff(reg, bit);
  }

  void M5IOE1_Class::enablePull(uint8_t pin, bool enablePull)
  {
    if (!_isValidPin(pin)) { return; }
    const auto pu_reg = _regForPin(M5IOE1_REG_GPIO_PU_L, pin);
    const auto pd_reg = _regForPin(M5IOE1_REG_GPIO_PD_L, pin);
    const auto bit = _bitForPin(pin);
    if (enablePull) {
      bitOn(pu_reg, bit);
    } else {
      bitOff(pu_reg, bit);
      bitOff(pd_reg, bit);
    }
  }

  void M5IOE1_Class::setPullMode(uint8_t pin, bool mode)
  {
    if (!_isValidPin(pin)) { return; }
    const auto pu_reg = _regForPin(M5IOE1_REG_GPIO_PU_L, pin);
    const auto pd_reg = _regForPin(M5IOE1_REG_GPIO_PD_L, pin);
    const auto bit = _bitForPin(pin);
    // false=pull-down, true=pull-up.
    mode ? bitOn(pu_reg, bit) : bitOff(pu_reg, bit);
    mode ? bitOff(pd_reg, bit) : bitOn(pd_reg, bit);
  }

  void M5IOE1_Class::setHighImpedance(uint8_t pin, bool enable)
  {
    if (!_isValidPin(pin)) { return; }
    // M5IOE1 exposes drive mode here: 0=push-pull, 1=open-drain.
    const auto reg = _regForPin(M5IOE1_REG_GPIO_DRV_L, pin);
    const auto bit = _bitForPin(pin);
    enable ? bitOn(reg, bit) : bitOff(reg, bit);
  }

  bool M5IOE1_Class::getWriteValue(uint8_t pin)
  {
    if (!_isValidPin(pin)) { return false; }
    return (readRegister8(_regForPin(M5IOE1_REG_GPIO_OUT_L, pin)) & _bitForPin(pin)) != 0;
  }

  void M5IOE1_Class::digitalWrite(uint8_t pin, bool level)
  {
    if (!_isValidPin(pin)) { return; }
    const auto reg = _regForPin(M5IOE1_REG_GPIO_OUT_L, pin);
    const auto bit = _bitForPin(pin);
    level ? bitOn(reg, bit) : bitOff(reg, bit);
  }

  bool M5IOE1_Class::digitalRead(uint8_t pin)
  {
    if (!_isValidPin(pin)) { return false; }
    return (readRegister8(_regForPin(M5IOE1_REG_GPIO_IN_L, pin)) & _bitForPin(pin)) != 0;
  }

  void M5IOE1_Class::setPwmFrequency(std::uint16_t frequency)
  {
    std::uint8_t data[2] = { static_cast<std::uint8_t>(frequency & 0xFF), static_cast<std::uint8_t>(frequency >> 8) };
    writeRegister(M5IOE1_REG_PWM_FREQ_L, data, sizeof(data));
  }

  void M5IOE1_Class::setPwmDuty(std::uint8_t channel, std::uint16_t duty12, bool enable, bool polarity)
  {
    if (channel > pwm_ch4) { return; }
    duty12 &= 0x0FFF;
    std::uint8_t high = static_cast<std::uint8_t>(duty12 >> 8);
    if (enable) { high |= M5IOE1_PWM_ENABLE; }
    if (polarity) { high |= M5IOE1_PWM_POLARITY; }
    std::uint8_t data[2] = { static_cast<std::uint8_t>(duty12 & 0xFF), high };
    writeRegister(static_cast<std::uint8_t>(M5IOE1_REG_PWM1_DUTY_L + channel * 2), data, sizeof(data));
  }

  void M5IOE1_Class::resetIrq()
  {
    std::uint8_t data[2] = { 0x00, 0x00 };
    writeRegister(M5IOE1_REG_GPIO_IS_L, data, sizeof(data));
  }

  void M5IOE1_Class::disableIrq()
  {
    std::uint8_t data[2] = { 0x00, 0x00 };
    writeRegister(M5IOE1_REG_GPIO_IE_L, data, sizeof(data));
  }

  void M5IOE1_Class::enableIrq()
  {
    std::uint8_t data[2] = { 0xFF, 0x3F };
    writeRegister(M5IOE1_REG_GPIO_IE_L, data, sizeof(data));
  }
}
