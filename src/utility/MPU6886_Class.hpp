// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_MPU6886_CLASS_H__
#define __M5_MPU6886_CLASS_H__

#include "I2C_Class.hpp"

namespace m5
{
  class MPU6886_Class : public I2C_Device
  {
  public:
    static constexpr std::uint8_t DEV_ID_MPU6886 = 0x19;
    static constexpr std::uint8_t DEV_ID_MPU6050 = 0x68;
    static constexpr std::uint8_t DEV_ID_MPU9250 = 0x71;

    enum Ascale
    { AFS_2G = 0
    , AFS_4G
    , AFS_8G
    , AFS_16G
    };

    enum Gscale
    { GFS_250DPS = 0
    , GFS_500DPS
    , GFS_1000DPS
    , GFS_2000DPS
    };

    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x68;

    MPU6886_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    bool begin(I2C_Class* i2c = nullptr);

    bool getAccelAdc(std::int16_t* ax, std::int16_t* ay, std::int16_t* az) const;
    bool getGyroAdc(std::int16_t* gx, std::int16_t* gy, std::int16_t* gz) const;

    bool getAccel(float* ax, float* ay, float* az) const;
    bool getGyro(float* gx, float* gy, float* gz) const;
    bool getTemp(float* t) const;

    bool setINTPinActiveLogic(bool level);

    std::uint8_t whoAmI(void) const { return _device_id; }

  protected:
    void setGyroFsr(Gscale scale);
    void setAccelFsr(Ascale scale);

    float aRes, gRes;
    Gscale _gscale;
    Ascale _ascale;
    std::uint8_t _device_id = 0;
  };
}

#endif
