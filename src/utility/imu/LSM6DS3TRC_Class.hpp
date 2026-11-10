// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_LSM6DS3TRC_CLASS_H__
#define __M5_LSM6DS3TRC_CLASS_H__

#include "IMU_Base.hpp"

namespace m5
{
  /// LSM6DS3TR-C accelerometer and gyroscope (WHO_AM_I = 0x6A).
  class LSM6DS3TRC_Class : public IMU_Base
  {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x6A;
    static constexpr std::uint8_t SECONDARY_ADDRESS = 0x6B;
    static constexpr std::uint8_t DEVICE_ID = 0x6A;

    virtual ~LSM6DS3TRC_Class();
    LSM6DS3TRC_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS,
                    std::uint32_t freq = 400000,
                    I2C_Class* i2c = &In_I2C);

    imu_spec_t begin(I2C_Class* i2c = nullptr) override;
    imu_spec_t getImuRawData(imu_raw_data_t* data) const override;
    void getConvertParam(imu_convert_param_t* param) const override;
    bool getTempAdc(int16_t* adc) const override;

    std::uint8_t whoAmI(void) const;

  private:
    static constexpr std::uint8_t REG_WHO_AM_I  = 0x0F;
    static constexpr std::uint8_t REG_CTRL1_XL  = 0x10;
    static constexpr std::uint8_t REG_CTRL2_G   = 0x11;
    static constexpr std::uint8_t REG_CTRL3_C   = 0x12;
    static constexpr std::uint8_t REG_STATUS    = 0x1E;
    static constexpr std::uint8_t REG_OUT_TEMP_L = 0x20;
    static constexpr std::uint8_t REG_OUTX_L_G  = 0x22;

    static constexpr std::uint8_t CTRL3_SW_RESET = 0x01;
    static constexpr std::uint8_t CTRL3_IF_INC   = 0x04;
    static constexpr std::uint8_t CTRL3_BDU      = 0x40;
    static constexpr std::uint8_t STATUS_XLDA    = 0x01;
    static constexpr std::uint8_t STATUS_GDA     = 0x02;
    static constexpr std::uint8_t STATUS_TDA     = 0x04;

    static constexpr std::uint8_t CTRL1_XL_104HZ_8G = 0x4C;
    static constexpr std::uint8_t CTRL2_G_104HZ_2000DPS = 0x4C;
  };
}

#endif
