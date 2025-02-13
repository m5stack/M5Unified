/**
 * @file pi4ioe5v6408.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-06-26
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef __M5_PI4IOE5V6408_H__
#define __M5_PI4IOE5V6408_H__

#include "m5unified_common.h"

#include "I2C_Class.hpp"

namespace m5
{
  // https://www.diodes.com/assets/Datasheets/PI4IOE5V6408.pdf
  class PI4IOE5V6408_Class : public m5::I2C_Device
  {
  public:
    PI4IOE5V6408_Class(std::uint8_t i2c_addr = 0x43, std::uint32_t freq = 400000, m5::I2C_Class* i2c = &m5::In_I2C)
    : I2C_Device(i2c_addr, freq, i2c)
    {}

    bool begin();

    // false input, true output
    void setDirection(uint8_t pin, bool direction);

    void enablePull(uint8_t pin, bool enablePull);

    // false down, true up
    void setPullMode(uint8_t pin, bool mode);

    void setHighImpedance(uint8_t pin, bool enable);

    void digitalWrite(uint8_t pin, bool level);

    bool digitalRead(uint8_t pin);

    void resetIrq();

    void disableIrq();

    void enableIrq();
  };
}

#endif
