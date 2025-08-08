// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_BQ27220_CLASS_H__
#define __M5_BQ27220_CLASS_H__

#include "../I2C_Class.hpp"

namespace m5
{
  class BQ27220_Class : public I2C_Device
  {
    bool read_MuxAddrdata(uint8_t regAddr, uint16_t subReg, uint8_t *regData = nullptr, uint8_t length = 0);
    bool opration_status_check(void);

  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x55;

    BQ27220_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 100000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    bool begin(void);

    int16_t getCurrent_mA(void);
    int16_t getVoltage_mV(void);

    float getCurrent_F(void);
    float getVoltage_F(void);
  };
}

#endif
