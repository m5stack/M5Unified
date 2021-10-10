// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_IMU_CLASS_H__
#define __M5_IMU_CLASS_H__

#include "I2C_Class.hpp"
#include "MPU6886_Class.hpp"
#include "SH200Q_Class.hpp"

namespace m5
{
  enum imu_t
  { imu_unknown
  , imu_sh200q
  , imu_mpu6886
  };

  class IMU_Class
  {
  public:
    bool begin(void);

    bool getAccel(float* ax, float* ay, float* az);
    bool getGyro(float* gx, float* gy, float* gz);
    bool getTemp(float *t);

    imu_t getType(void) const { return _imu; }

    MPU6886_Class Mpu6886;
    SH200Q_Class Sh200q;
  private:
    imu_t _imu = imu_t::imu_unknown;
  };
}

#endif
