// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "SH200Q_Class.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace m5
{
  bool SH200Q_Class::begin(I2C_Class* i2c)
  {
    if (i2c)
    {
      _i2c = i2c;
    }

    // WHO_AM_I : IMU Check
    if (WhoAmI() != 0x18)
    {
      return false;
    }
    vTaskDelay(1);

    bitOn(0xC2, 0x04);
    vTaskDelay(1);

    bitOff(0xC2, 0x04);
    vTaskDelay(1);

    bitOn(0xD8, 0x80);
    vTaskDelay(1);

    bitOff(0xD8, 0x80);
    vTaskDelay(1);

    static constexpr std::uint8_t init_cmd[] =
    { 0x78, 0x61
    , 0x78, 0x00
    , 0x0E, 0x91  // ACC_CONFIG(0x0E) : 256Hz
    , 0x0F, 0x13  // GYRO_CONFIG(0x0F) : 500Hz
    , 0x11, 0x03  // GYRO_DLPF(0x11) : 50Hz
    , 0x12, 0x00  // FIFO_CONFIG(0x12)
    , 0x16, 0x01  // ACC_RANGE(0x16) : +-8G
    , 0x2B, 0x00  // GYRO_RANGE(0x2B) : +-2000
    , 0xBA, 0xC0  // REG_SET1(0xBA)
    , 0xFF, 0xFF
    };

    for (int idx = -1;;)
    {
      std::uint8_t reg = init_cmd[++idx];
      std::uint8_t val = init_cmd[++idx];
      if ((reg & val) == 0xFF) { break; }
      writeRegister8(reg, val);
      vTaskDelay(1);
    }

    // REG_SET2(0xCA)
    bitOn(0xCA, 0x10);
    vTaskDelay(1);

    // REG_SET2(0xCA)
    bitOff(0xCA, 0x10);
    vTaskDelay(1);

    _init = true;
    return true;
  }

  std::uint8_t SH200Q_Class::WhoAmI(void)
  {
    return readRegister8(0x30);
  }

  bool SH200Q_Class::getAccelAdc(std::int16_t* ax, std::int16_t* ay, std::int16_t* az) const
  {
    std::int16_t buf[3];
    bool res = readRegister(0x00, (std::uint8_t*)buf, 6);
    *ax = buf[0];
    *ay = buf[1];
    *az = buf[2];
    return res;
  }

  bool SH200Q_Class::getGyroAdc(std::int16_t* gx, std::int16_t* gy, std::int16_t* gz) const
  {
    std::int16_t buf[3];
    bool res = readRegister(0x06, (std::uint8_t*)buf, 6);
    *gx = buf[0];
    *gy = buf[1];
    *gz = buf[2];
    return res;
  }

  bool SH200Q_Class::getAccel(float* ax, float* ay, float* az) const
  {
    static constexpr float aRes = 8.0f / 32768.0f;
    std::int16_t buf[3];
    bool res = readRegister(0x00, (std::uint8_t*)buf, 6);
    *ax = buf[0] * aRes;
    *ay = buf[1] * aRes;
    *az = buf[2] * aRes;
    return res;
  }

  bool SH200Q_Class::getGyro(float* gx, float* gy, float* gz) const
  {
    static constexpr float gRes = 2000.0f / 32768.0f;
    std::int16_t buf[3];
    bool res = readRegister(0x06, (std::uint8_t*)buf, 6);
    *gx = buf[0] * gRes;
    *gy = buf[1] * gRes;
    *gz = buf[2] * gRes;
    return res;
  }

  bool SH200Q_Class::getTemp(float *t) const
  {
    std::int16_t buf;
    bool res = readRegister(0x0C, (std::uint8_t*)&buf, 2);
    *t = 21.0f + buf / 333.87f;
    return res;
  }
}
