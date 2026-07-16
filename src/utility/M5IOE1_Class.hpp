// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_M5IOE1_CLASS_H__
#define __M5_M5IOE1_CLASS_H__

#include "IOExpander_Base.hpp"

namespace m5
{
  class M5IOE1_Class : public IOExpander_Base
  {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x4F;
    static constexpr std::uint32_t DEFAULT_FREQ = 100000;

    enum gpio_t : std::uint8_t
    { gpio1  = 0
    , gpio2  = 1
    , gpio3  = 2
    , gpio4  = 3
    , gpio5  = 4
    , gpio6  = 5
    , gpio7  = 6
    , gpio8  = 7
    , gpio9  = 8
    , gpio10 = 9
    , gpio11 = 10
    , gpio12 = 11
    , gpio13 = 12
    , gpio14 = 13
    };

    enum pwm_channel_t : std::uint8_t
    { pwm_ch1 = 0 // IO_9
    , pwm_ch2 = 1 // IO_8
    , pwm_ch3 = 2 // IO_11
    , pwm_ch4 = 3 // IO_10
    };

    M5IOE1_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = DEFAULT_FREQ, m5::I2C_Class* i2c = &m5::In_I2C)
    : IOExpander_Base(i2c_addr, freq, i2c)
    {}

    bool begin();

    void setDirection(uint8_t pin, bool direction) override;

    void enablePull(uint8_t pin, bool enablePull) override;

    void setPullMode(uint8_t pin, bool mode) override;

    void setHighImpedance(uint8_t pin, bool enable) override;

    bool getWriteValue(uint8_t pin) override;

    void digitalWrite(uint8_t pin, bool level) override;

    bool digitalRead(uint8_t pin) override;

    void setPwmFrequency(std::uint16_t frequency);

    void setPwmDuty(std::uint8_t channel, std::uint16_t duty12, bool enable = true, bool polarity = false);

    void resetIrq() override;

    void disableIrq() override;

    void enableIrq() override;

  private:
    static bool _isValidPin(uint8_t pin) { return pin < 14; }
    static std::uint8_t _regForPin(std::uint8_t reg_low, std::uint8_t pin) { return reg_low + (pin >> 3); }
    static std::uint8_t _bitForPin(std::uint8_t pin) { return 1 << (pin & 7); }
  };
}

#endif
