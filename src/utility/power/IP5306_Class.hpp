// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_IP5306_CLASS_H__
#define __M5_IP5306_CLASS_H__

#include "../I2C_Class.hpp"

namespace m5
{
  class IP5306_Class : public I2C_Device
  {
  public:

    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x75;

    IP5306_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    bool begin(void);

    /// Get the remaining battery power.
    /// @return 0-100 level
    std::int8_t getBatteryLevel(void);

    /// set battery charge enable.
    /// @param enable true=enable / false=disable
    /// @return false on I2C failure.
    bool setBatteryCharge(bool enable);

    /// get the charge enable setting. (SYS_CTL0 bit4)
    /// @param enabled output parameter, receives the charge enable setting.
    /// @return false on I2C failure.
    /// @note This is the value that was written, which survives an unplugged
    /// supply. readChargeActive() reports the effective one.
    bool getBatteryCharge(bool* enabled);

    /// read the effective charge enable flag. (REG_READ0 bit3)
    /// @param active output parameter. false when charging is disabled or no supply is present.
    /// @return false on I2C failure.
    /// @note This flag is "charging is switched on", not "the cell is filling":
    /// it stays set after the charge completes. Pair it with readChargeFull().
    bool readChargeActive(bool* active);

    /// read the charge complete flag. (REG_READ1 bit3)
    /// @param full output parameter, receives the charge complete flag.
    /// @return false on I2C failure.
    /// @note The flag has no defined reset value, so shortly after power up it
    /// can read as full before the charger has settled.
    bool readChargeFull(bool* full);

    /// set battery charge current
    /// @param max_mA milli ampere. (50 - 3150, in steps of 100mA over a 50mA floor).
    /// @param applied_mA optional. receives the step that was applied.
    /// @return false on I2C failure. applied_mA is left untouched then.
    bool setChargeCurrent(std::uint16_t max_mA, std::uint16_t* applied_mA = nullptr);

    /// set battery charge voltage
    /// @param max_mV milli volt. The steps are the effective values including
    /// the constant-voltage boost the datasheet recommends: 4228 / 4314 /
    /// 4364 / 4414 mV. The highest step not above max_mV is selected, and a
    /// request under the lowest step selects that step.
    /// @param applied_mV optional. receives the effective step that was applied.
    /// @return false on I2C failure. applied_mV is left untouched then.
    bool setChargeVoltage(std::uint16_t max_mV, std::uint16_t* applied_mV = nullptr);

    /// Get whether the battery is currently charging or not.
    bool isCharging(void);

    /// Set whether or not to continue supplying power even at low loads.
    bool setPowerBoostKeepOn(bool en);

  private:
  };
}

#endif
