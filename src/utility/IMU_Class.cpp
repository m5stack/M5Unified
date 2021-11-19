// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "IMU_Class.hpp"

#include "../M5Unified.hpp"

namespace m5
{
  bool IMU_Class::begin(I2C_Class* i2c)
  {
    if (i2c)
    {
      i2c->begin();
    }

    _imu = imu_t::imu_unknown;
    if (Mpu6886.begin(i2c))
    {
      switch (Mpu6886.whoAmI())
      {
      case MPU6886_Class::DEV_ID_MPU6050:
        _imu = imu_t::imu_mpu6050;
        break;
      case MPU6886_Class::DEV_ID_MPU6886:
        _imu = imu_t::imu_mpu6886;
        break;
      case MPU6886_Class::DEV_ID_MPU9250:
        _imu = imu_t::imu_mpu9250;
        break;
      default:
        return false;
      }
    }
    else
    if (Sh200q.begin(i2c))
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
    bool res = false;
    if (_imu == imu_t::imu_unknown)
    {
    }
    if (_imu == imu_t::imu_sh200q)
    {
      res = Sh200q.getAccel(x, y, z);
    }
    else
    // if (_imu == imu_t::imu_mpu6050
    //  || _imu == imu_t::imu_mpu6886
    //  || _imu == imu_t::imu_mpu9250)
    {
      res = Mpu6886.getAccel(x, y, z);
    }
    if (!res)
    {
      *x = 0;
      *y = 0;
      *z = 0;
    }
    else
    {
      auto r = _rotation;
      if (r)
      {
        if (r == 2)
        {
          *x = -(*x);
          *z = -(*z);
        }
        else
        {
          auto tmpx = *x;
          if (r == 1)
          {
            *x = - *z;
            *z = tmpx;
          }
          else
          {
            *x = *z;
            *z = - tmpx;
          }
        }
      }
    }
    return res;
  }

  bool IMU_Class::getGyro(float *x, float *y, float *z)
  {
    bool res = false;
    if (_imu == imu_t::imu_unknown)
    {
    }
    if (_imu == imu_t::imu_sh200q)
    {
      res = Sh200q.getGyro(x, y, z);
    }
    else
    // if (_imu == imu_t::imu_mpu6050
    //  || _imu == imu_t::imu_mpu6886
    //  || _imu == imu_t::imu_mpu9250)
    {
      res = Mpu6886.getGyro(x, y, z);
    }
    if (!res)
    {
      *x = 0;
      *y = 0;
      *z = 0;
    }
    else
    {
      auto r = _rotation;
      if (r)
      {
        if (r == 2)
        {
          *x = -(*x);
          *z = -(*z);
        }
        else
        {
          auto tmpx = *x;
          if (r == 1)
          {
            *x = - *z;
            *z = tmpx;
          }
          else
          {
            *x = *z;
            *z = - tmpx;
          }
        }
      }
    }
    return res;
  }
}
