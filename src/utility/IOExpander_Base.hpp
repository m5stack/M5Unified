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
    IOExpander_Base(std::uint8_t i2c_addr, std::uint32_t freq = 400000, m5::I2C_Class* i2c = &m5::In_I2C)
      : I2C_Device(i2c_addr, freq, i2c)
    {}
    IOExpander_Base(const IOExpander_Base&) = delete;

    // false input, true output
    virtual void setDirection(uint8_t pin, bool direction) = 0;

    virtual void enablePull(uint8_t pin, bool enablePull) = 0;

    // false down, true up
    virtual void setPullMode(uint8_t pin, bool mode) = 0;

    virtual void setHighImpedance(uint8_t pin, bool enable) = 0;

    virtual bool getWriteValue(uint8_t pin) = 0;

    virtual void digitalWrite(uint8_t pin, bool level) = 0;

    virtual bool digitalRead(uint8_t pin) = 0;

    virtual void resetIrq() = 0;

    virtual void disableIrq() = 0;

    virtual void enableIrq() = 0;
  };
}

#endif
