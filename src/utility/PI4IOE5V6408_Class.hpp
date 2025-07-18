/**
 * @file pi4ioe5v6408.h
 * @author Forairaaaaa, lovyan03
 * @brief
 * @version 0.2
 * @date 2025-06-11
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef __M5_PI4IOE5V6408_H__
#define __M5_PI4IOE5V6408_H__

#include "m5unified_common.h"

#include "IOExpander_Base.hpp"
#include "I2C_Class.hpp"

namespace m5
{
  // https://www.diodes.com/assets/Datasheets/PI4IOE5V6408.pdf
  class PI4IOE5V6408_Class : public IOExpander_Base
  {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x43;

    PI4IOE5V6408_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, m5::I2C_Class* i2c = &m5::In_I2C)
    : IOExpander_Base(i2c_addr, freq, i2c)
    {}

    bool begin();

    // false input, true output
    void setDirection(uint8_t pin, bool direction) override;

    void enablePull(uint8_t pin, bool enablePull) override;

    // false down, true up
    void setPullMode(uint8_t pin, bool mode) override;

    void setHighImpedance(uint8_t pin, bool enable) override;

    bool getWriteValue(uint8_t pin) override;

    void digitalWrite(uint8_t pin, bool level) override;

    bool digitalRead(uint8_t pin) override;

    void resetIrq() override;

    void disableIrq() override;

    void enableIrq() override;
  };
}

#endif
