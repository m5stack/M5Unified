// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "IMU_Class.hpp"

namespace m5
{
  bool IMU_Class::begin(void)
  {
    if (Mpu6886.begin())
    {
      _imu = imu_t::imu_mpu6886;
    }
    else
    if (Sh200q.begin())
    {
      _imu = imu_t::imu_sh200q;
    }
    else
    {
      _imu = imu_t::imu_unknown;
      return false;
    }
    return true;
  }

  bool IMU_Class::getAccel(float *x, float *y, float *z)
  {
    if (_imu == imu_t::imu_mpu6886)
    {
      Mpu6886.getAccel(x, y, z);
    }
    else if (_imu == imu_t::imu_sh200q)
    {
      Sh200q.getAccel(x, y, z);
    }
    else
    {
      *x = 0;
      *y = 0;
      *z = 0;
      return false;
    }
/*
    if (board == m5gfx::board_unknown) {
      *x = -(*x);
      *z = -(*z);
    }
//*/
    return true;
  }

  bool IMU_Class::getGyro(float *x, float *y, float *z)
  {
    if (_imu == imu_t::imu_mpu6886)
    {
      Mpu6886.getGyro(x, y, z);
    }
    else if (_imu == imu_t::imu_sh200q)
    {
      Sh200q.getGyro(x, y, z);
    }
    else
    {
      *x = 0;
      *y = 0;
      *z = 0;
      return false;
    }
/*
    if (board == m5gfx::board_unknown) {
      *x = -(*x);
      *z = -(*z);
    }
//*/
    return true;
  }

}
