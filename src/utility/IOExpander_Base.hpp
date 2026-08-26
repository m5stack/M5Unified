// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_IOEXPANDER_BASE_H__
#define __M5_IOEXPANDER_BASE_H__

#include <stdint.h>
#include "I2C_Class.hpp"

namespace m5
{
  class IOExpander_Base : public I2C_Device
  {
  public:
    enum gpio_pull_t : std::uint8_t
    { pull_none = 0
    , pull_up   = 1
    , pull_down = 2
    };

    /// Mutating methods report their result: the written value is not read
    /// back, and read-modify-write accesses are not atomic against
    /// concurrent access to the same register.

    IOExpander_Base(std::uint8_t i2c_addr, std::uint32_t freq = 400000, m5::I2C_Class* i2c = &m5::In_I2C)
      : I2C_Device(i2c_addr, freq, i2c)
    {}
    IOExpander_Base(const IOExpander_Base&) = delete;

    // false input, true output
    /// @return true when every required register access was acknowledged;
    /// false for an invalid pin or I2C failure.
    virtual bool setDirection(uint8_t pin, bool direction) = 0;

    /// Set the GPIO pull resistor state.
    /// @return true when every required register access was acknowledged;
    /// false for an invalid pin, mode, or I2C failure.
    virtual bool setPullMode(uint8_t pin, gpio_pull_t mode) = 0;

    /// @return true when every required register access was acknowledged;
    /// false for an invalid pin or I2C failure.
    virtual bool setHighImpedance(uint8_t pin, bool enable) = 0;

    virtual bool getWriteValue(uint8_t pin) = 0;

    /// @return true when every required register access was acknowledged;
    /// false for an invalid pin or I2C failure.
    virtual bool digitalWrite(uint8_t pin, bool level) = 0;

    virtual bool digitalRead(uint8_t pin) = 0;

    /// digitalRead with I2C error reporting. returns false on I2C failure
    /// or when the expander driver does not implement error detection.
    virtual bool getInputLevel(uint8_t pin, bool* level)
    {
      (void)pin;
      (void)level;
      return false;
    }

    /// @return true when every required register access was acknowledged;
    /// false on I2C failure.
    virtual bool resetIrq() = 0;

    /// @return true when every required register access was acknowledged;
    /// false on I2C failure.
    virtual bool disableIrq() = 0;

    /// @return true when every required register access was acknowledged;
    /// false on I2C failure.
    virtual bool enableIrq() = 0;
  };
}

#endif
