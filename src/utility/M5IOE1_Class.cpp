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

  bool M5IOE1_Class::setDirection(uint8_t pin, bool direction)
  {
    if (!_isValidPin(pin)) { return false; }
    // false=input, true=output. MODE bit set means output.
    const auto reg = _regForPin(M5IOE1_REG_GPIO_MODE_L, pin);
    const auto bit = _bitForPin(pin);
    return direction ? bitOn(reg, bit) : bitOff(reg, bit);
  }

  bool M5IOE1_Class::setPullMode(uint8_t pin, gpio_pull_t mode)
  {
    if (!_isValidPin(pin)) { return false; }
    const auto pu_reg = _regForPin(M5IOE1_REG_GPIO_PU_L, pin);
    const auto pd_reg = _regForPin(M5IOE1_REG_GPIO_PD_L, pin);
    const auto bit = _bitForPin(pin);
    switch (mode) {
    case pull_none: {
      const bool pu_ok = bitOff(pu_reg, bit);
      const bool pd_ok = bitOff(pd_reg, bit);
      return pu_ok && pd_ok;
    }
    case pull_up:
      if (!bitOff(pd_reg, bit)) { return false; }
      return bitOn(pu_reg, bit);
    case pull_down:
      if (!bitOff(pu_reg, bit)) { return false; }
      return bitOn(pd_reg, bit);
    default:
      return false;
    }
  }

  bool M5IOE1_Class::setHighImpedance(uint8_t pin, bool enable)
  {
    if (!_isValidPin(pin)) { return false; }
    // M5IOE1 exposes drive mode here: 0=push-pull, 1=open-drain.
    const auto reg = _regForPin(M5IOE1_REG_GPIO_DRV_L, pin);
    const auto bit = _bitForPin(pin);
    return enable ? bitOn(reg, bit) : bitOff(reg, bit);
  }

  bool M5IOE1_Class::getWriteValue(uint8_t pin)
  {
    if (!_isValidPin(pin)) { return false; }
    return (readRegister8(_regForPin(M5IOE1_REG_GPIO_OUT_L, pin)) & _bitForPin(pin)) != 0;
  }

  bool M5IOE1_Class::digitalWrite(uint8_t pin, bool level)
  {
    if (!_isValidPin(pin)) { return false; }
    const auto reg = _regForPin(M5IOE1_REG_GPIO_OUT_L, pin);
    const auto bit = _bitForPin(pin);
    return level ? bitOn(reg, bit) : bitOff(reg, bit);
  }

  bool M5IOE1_Class::digitalRead(uint8_t pin)
  {
    if (!_isValidPin(pin)) { return false; }
    return (readRegister8(_regForPin(M5IOE1_REG_GPIO_IN_L, pin)) & _bitForPin(pin)) != 0;
  }

  bool M5IOE1_Class::getInputLevel(uint8_t pin, bool* level)
  {
    if (!_isValidPin(pin) || level == nullptr) { return false; }
    std::uint8_t v;
    if (!readRegister(_regForPin(M5IOE1_REG_GPIO_IN_L, pin), &v, 1)) { return false; }
    *level = (v & _bitForPin(pin)) != 0;
    return true;
  }

  bool M5IOE1_Class::setPwmFrequency(std::uint16_t frequency)
  {
    std::uint8_t data[2] = { static_cast<std::uint8_t>(frequency & 0xFF), static_cast<std::uint8_t>(frequency >> 8) };
    return writeRegister(M5IOE1_REG_PWM_FREQ_L, data, sizeof(data));
  }

  bool M5IOE1_Class::setPwmDutyPercent(pwm_channel_t channel, std::uint32_t duty,
                                       pwm_polarity_t polarity, bool enable)
  {
    if (duty > 100) { return false; }
    auto duty12 = duty * 0x0FFF / 100;
    return setPwmDuty12bit(channel, duty12, polarity, enable);
  }

  bool M5IOE1_Class::setPwmDuty12bit(pwm_channel_t channel, std::uint32_t duty12,
                                     pwm_polarity_t polarity, bool enable)
  {
    if (channel > pwm_ch4 || duty12 > 0x0FFF) { return false; }
    std::uint8_t high = static_cast<std::uint8_t>(duty12 >> 8);
    if (enable) { high |= M5IOE1_PWM_ENABLE; }
    if (polarity == pwm_polarity_t::inverted) { high |= M5IOE1_PWM_POLARITY; }
    std::uint8_t data[2] = { static_cast<std::uint8_t>(duty12 & 0xFF), high };
    auto reg = static_cast<std::uint8_t>(M5IOE1_REG_PWM1_DUTY_L + static_cast<std::uint8_t>(channel) * 2);
    return writeRegister(reg, data, sizeof(data));
  }

  bool M5IOE1_Class::resetIrq()
  {
    std::uint8_t data[2] = { 0x00, 0x00 };
    return writeRegister(M5IOE1_REG_GPIO_IS_L, data, sizeof(data));
  }

  bool M5IOE1_Class::disableIrq()
  {
    std::uint8_t data[2] = { 0x00, 0x00 };
    return writeRegister(M5IOE1_REG_GPIO_IE_L, data, sizeof(data));
  }

  bool M5IOE1_Class::enableIrq()
  {
    std::uint8_t data[2] = { 0xFF, 0x3F };
    return writeRegister(M5IOE1_REG_GPIO_IE_L, data, sizeof(data));
  }
}
