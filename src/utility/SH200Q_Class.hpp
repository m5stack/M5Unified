// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_SH200Q_CLASS_H__
#define __M5_SH200Q_CLASS_H__

#include "I2C_Class.hpp"

namespace m5
{
  class SH200Q_Class : public I2C_Device
  {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x6C;

    SH200Q_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    bool begin(I2C_Class* i2c = nullptr);

    std::uint8_t WhoAmI(void);

    bool getAccelAdc(std::int16_t* ax, std::int16_t* ay, std::int16_t* az) const;
    bool getGyroAdc(std::int16_t* gx, std::int16_t* gy, std::int16_t* gz) const;

    bool getAccel(float* ax, float* ay, float* az) const;
    bool getGyro(float* gx, float* gy, float* gz) const;
    bool getTemp(float *t) const;
  };
}

#endif
