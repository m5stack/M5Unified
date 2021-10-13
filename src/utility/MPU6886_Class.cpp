// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "MPU6886_Class.hpp"

#include <esp_log.h>

namespace m5
{

  bool MPU6886_Class::begin(void)
  {
    // WHO_AM_I : IMU Check
    _device_id = readRegister8(0x75);
    if (_device_id != DEV_ID_MPU6886
     && _device_id != DEV_ID_MPU6050
     && _device_id != DEV_ID_MPU9250)
    {
      return false;
    }

    vTaskDelay(1);

    // PWR_MGMT_1(0x6B)
    writeRegister8(0x6B, 0x00);
    vTaskDelay(10);

    // PWR_MGMT_1(0x6B)
    writeRegister8(0x6B, 0x80);
    vTaskDelay(10);

    // PWR_MGMT_1(0x6B)
    writeRegister8(0x6B, 0x01);
    vTaskDelay(10);

    static constexpr std::uint8_t init_cmd[] =
    { 0x1C, 0x10  // ACCEL_CONFIG(0x1C) : +-8G
    , 0x1B, 0x18  // GYRO_CONFIG(0x1B) : +-2000dps
    , 0x1A, 0x01  // CONFIG(0x1A)
    , 0x19, 0x05  // SMPLRT_DIV(0x19)
    , 0x38, 0x00  // INT_ENABLE(0x38)
    , 0x1D, 0x00  // ACCEL_CONFIG 2(0x1D)
    , 0x6A, 0x00  // USER_CTRL(0x6A)
    , 0x23, 0x00  // FIFO_EN(0x23)
    , 0x37, 0x22  // INT_PIN_CFG(0x37)
    , 0x38, 0x01  // INT_ENABLE(0x38)
    , 0xFF, 0xFF  // EOF
    };

    for (int idx = -1;;)
    {
      std::uint8_t reg = init_cmd[++idx];
      std::uint8_t val = init_cmd[++idx];
      if ((reg & val) == 0xFF) { break; }
      writeRegister8(reg, val);
      vTaskDelay(1);
    }

    setGyroFsr(Gscale::GFS_2000DPS);
    setAccelFsr(Ascale::AFS_8G);

    _init = true;
    return _init;
  }

  void MPU6886_Class::setGyroFsr(Gscale scale)
  {
    _gscale = scale;
    writeRegister8(0x1B, scale << 3);
    vTaskDelay(10);
    switch (scale)
    {
    case GFS_250DPS:
      gRes = 250.0f / 32768.0f;
      break;
    case GFS_500DPS:
      gRes = 500.0f / 32768.0f;
      break;
    case GFS_1000DPS:
      gRes = 1000.0f / 32768.0f;
      break;
    case GFS_2000DPS:
      gRes = 2000.0f / 32768.0f;
      break;
    }
  }

  void MPU6886_Class::setAccelFsr(Ascale scale)
  {
    _ascale = scale;
    writeRegister8(0x1C, scale << 3);
    vTaskDelay(10);
    switch (scale)
    {
    case AFS_2G:
      aRes = 2.0f / 32768.0f;
      break;
    case AFS_4G:
      aRes = 4.0f / 32768.0f;
      break;
    case AFS_8G:
      aRes = 8.0f / 32768.0f;
      break;
    case AFS_16G:
      aRes = 16.0f / 32768.0f;
      break;
    }
  }

  bool MPU6886_Class::setINTPinActiveLogic(bool level)
  {
    // MPU6886_INT_PIN_CFG = 0x37
    std::uint8_t tmp = readRegister8(0x37) & 0x7F;
    tmp |= level ? 0x00 : 0x80;
    return writeRegister8(0x37, tmp);
  }

  void MPU6886_Class::getAccelAdc(std::int16_t* ax, std::int16_t* ay, std::int16_t* az) const
  {
    std::uint8_t buf[6];
    readRegister(0x3B, buf, 6); // MPU6886_ACCEL
    *ax = (buf[0] << 8) | buf[1];
    *ay = (buf[2] << 8) | buf[3];
    *az = (buf[4] << 8) | buf[5];
  }

  void MPU6886_Class::getGyroAdc(std::int16_t* gx, std::int16_t* gy, std::int16_t* gz) const
  {
    std::uint8_t buf[6];
    readRegister(0x43, buf, 6); // MPU6886_GYRO
    *gx = (buf[0] << 8) | buf[1];
    *gy = (buf[2] << 8) | buf[3];
    *gz = (buf[4] << 8) | buf[5];
  }

  void MPU6886_Class::getAccel(float* ax, float* ay, float* az)
  {
    static constexpr float aRes = 8.0f / 32768.0f;
    std::uint8_t buf[6];
    readRegister(0x3B, buf, 6);
    *ax = (std::int16_t)((buf[0] << 8) | buf[1]) * aRes;
    *ay = (std::int16_t)((buf[2] << 8) | buf[3]) * aRes;
    *az = (std::int16_t)((buf[4] << 8) | buf[5]) * aRes;
  }

  void MPU6886_Class::getGyro(float* gx, float* gy, float* gz)
  {
    static constexpr float gRes = 2000.0f / 32768.0f;
    std::uint8_t buf[6];
    readRegister(0x43, buf, 6);
    *gx = (std::int16_t)((buf[0] << 8) | buf[1]) * gRes;
    *gy = (std::int16_t)((buf[2] << 8) | buf[3]) * gRes;
    *gz = (std::int16_t)((buf[4] << 8) | buf[5]) * gRes;
  }

  void MPU6886_Class::getTemp(float *t)
  {
    std::uint8_t buf[2];
    readRegister(0x41, buf, 2);
    *t = 25.0f + ((buf[0] << 8) | buf[1]) / 326.8f;
  }
}
