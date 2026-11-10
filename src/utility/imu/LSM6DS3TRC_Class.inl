// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_IMPLEMENTATION
#error "LSM6DS3TRC_Class.inl is part of M5Unified.cpp and is not meant to be included on its own"
#endif

#if defined (ESP_PLATFORM)

#include "LSM6DS3TRC_Class.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace m5
{
  LSM6DS3TRC_Class::~LSM6DS3TRC_Class() {}

  LSM6DS3TRC_Class::LSM6DS3TRC_Class(std::uint8_t i2c_addr, std::uint32_t freq, I2C_Class* i2c)
  : IMU_Base ( i2c_addr, freq, i2c )
  {}

  IMU_Base::imu_spec_t LSM6DS3TRC_Class::begin(I2C_Class* i2c)
  {
    if (i2c)
    {
      _i2c = i2c;
    }
    _init = false;

    // The device loads its trimming parameters during the power-on boot phase.
    vTaskDelay(pdMS_TO_TICKS(15) + 1);
    std::uint8_t value;
    if (!readRegister(REG_WHO_AM_I, &value, 1) || value != DEVICE_ID)
    {
      return imu_spec_none;
    }

    if (!writeRegister8(REG_CTRL3_C, CTRL3_SW_RESET))
    {
      return imu_spec_none;
    }

    const TickType_t timeout = pdMS_TO_TICKS(100) + 1;
    const TickType_t reset_start = xTaskGetTickCount();
    do
    {
      vTaskDelay(1);
      if (!readRegister(REG_CTRL3_C, &value, 1))
      {
        return imu_spec_none;
      }
      if ((value & CTRL3_SW_RESET) == 0)
      {
        break;
      }
    } while (xTaskGetTickCount() - reset_start < timeout);
    if (value & CTRL3_SW_RESET)
    {
      return imu_spec_none;
    }

    const std::uint8_t ctrl3 = CTRL3_BDU | CTRL3_IF_INC;
    if (!writeRegister8(REG_CTRL3_C, ctrl3)
     || !writeRegister8(REG_CTRL1_XL, CTRL1_XL_104HZ_8G)
     || !writeRegister8(REG_CTRL2_G, CTRL2_G_104HZ_2000DPS))
    {
      return imu_spec_none;
    }

    std::uint8_t config[3];
    if (!readRegister(REG_CTRL1_XL, config, sizeof(config))
     || config[0] != CTRL1_XL_104HZ_8G
     || config[1] != CTRL2_G_104HZ_2000DPS
     || config[2] != ctrl3)
    {
      return imu_spec_none;
    }

    // AN5130 section 3.9: 70 ms gyro turn-on plus 3 discarded samples at
    // 104 Hz. BDU may still hold a sample from before turn-on completed.
    vTaskDelay(pdMS_TO_TICKS(70) + 1);
    std::uint8_t discarded[12];
    if (!readRegister(REG_OUTX_L_G, discarded, sizeof(discarded)))
    {
      return imu_spec_none;
    }
    unsigned samples_to_discard = 3;

    // IMU_Class calibrates immediately after begin(), so leave a complete,
    // settled sample ready in the output registers before reporting success.
    const TickType_t sample_start = xTaskGetTickCount();
    do
    {
      vTaskDelay(1);
      if (!readRegister(REG_STATUS, &value, 1))
      {
        return imu_spec_none;
      }
      if ((value & (STATUS_XLDA | STATUS_GDA)) == (STATUS_XLDA | STATUS_GDA))
      {
        if (samples_to_discard == 0)
        {
          _init = true;
          return (imu_spec_t)(imu_spec_accel | imu_spec_gyro);
        }
        if (!readRegister(REG_OUTX_L_G, discarded, sizeof(discarded)))
        {
          return imu_spec_none;
        }
        --samples_to_discard;
      }
    } while (xTaskGetTickCount() - sample_start < timeout);

    return imu_spec_none;
  }

  IMU_Base::imu_spec_t LSM6DS3TRC_Class::getImuRawData(imu_raw_data_t* data) const
  {
    std::uint8_t status;
    if (!readRegister(REG_STATUS, &status, 1))
    {
      return imu_spec_none;
    }

    const std::uint8_t ready = status & (STATUS_XLDA | STATUS_GDA);
    if (ready == 0)
    {
      return imu_spec_none;
    }

    std::int16_t raw[6];
    if (!readRegister(REG_OUTX_L_G, reinterpret_cast<std::uint8_t*>(raw), sizeof(raw)))
    {
      return imu_spec_none;
    }

    imu_spec_t result = imu_spec_none;
    if (ready & STATUS_GDA)
    {
      data->gyro.x = raw[0];
      data->gyro.y = raw[1];
      data->gyro.z = raw[2];
      result = (imu_spec_t)(result | imu_spec_gyro);
    }
    if (ready & STATUS_XLDA)
    {
      data->accel.x = raw[3];
      data->accel.y = raw[4];
      data->accel.z = raw[5];
      result = (imu_spec_t)(result | imu_spec_accel);
    }
    return result;
  }

  void LSM6DS3TRC_Class::getConvertParam(imu_convert_param_t* param) const
  {
    param->accel_res = 0.000244f;
    param->gyro_res = 0.070f;
    param->temp_res = 1.0f / 256.0f;
    param->temp_offset = 25.0f;
  }

  bool LSM6DS3TRC_Class::getTempAdc(int16_t* adc) const
  {
    std::uint8_t status;
    if (!readRegister(REG_STATUS, &status, 1) || !(status & STATUS_TDA))
    {
      return false;
    }
    return readRegister(REG_OUT_TEMP_L, reinterpret_cast<std::uint8_t*>(adc), sizeof(*adc));
  }

  std::uint8_t LSM6DS3TRC_Class::whoAmI(void) const
  {
    return readRegister8(REG_WHO_AM_I);
  }
}

#endif
