// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_PY32PMIC_CLASS_H__
#define __M5_PY32PMIC_CLASS_H__

#include "../I2C_Class.hpp"

namespace m5
{
  class PY32PMIC_Class : public I2C_Device
  {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x6E;

    PY32PMIC_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 100000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    bool begin(void);

    // true=output / false=input
    bool setExtOutput(bool enable);

    /// set battery charge enable.
    /// @param enable true=enable / false=disable
    bool setBatteryCharge(bool enable);

    /// set battery charge current
    /// @param max_mA milli ampere. (8 - 512).
    bool setChargeCurrent(std::uint16_t max_mA);

    /// set battery charge voltage
    /// @param max_mV milli volt. (3600 - 4545).
    bool setChargeVoltage(std::uint16_t max_mV);

    /// Get whether the battery is currently charging or not.
    bool isCharging(void);

    // get setting value of battery charge current
    /// @return milli ampere. (8 - 512). 0=unknown
    std::uint16_t getChargeCurrent(void);

    // get setting value of battery charge voltage
    /// @return milli volt. (3600 - 4545). 0=unknown
    std::uint16_t getChargeVoltage(void);

    uint8_t getPekPress(void);

    bool powerOff(void);
  };
}

#endif
