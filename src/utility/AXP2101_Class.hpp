// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_AXP2101_CLASS_H__
#define __M5_AXP2101_CLASS_H__

#include "I2C_Class.hpp"

namespace m5
{
  class AXP2101_Class : public I2C_Device
  {
  public:
    static constexpr uint8_t AXP2101_EFUS_OP_CFG   = 0xF0;
    static constexpr uint8_t AXP2101_EFREQ_CTRL    = 0xF1;
    static constexpr uint8_t AXP2101_TWI_ADDR_EXT  = 0xFF;

    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x34;

    AXP2101_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    bool begin(void);

    /// Get the remaining battery power.
    /// @return 0-100 level
    std::int8_t getBatteryLevel(void);

    /// set battery charge enable.
    /// @param enable true=enable / false=disable
    void setBatteryCharge(bool enable);

    /// set battery charge current
    /// @param max_mA milli ampere. (100 - 1320).
    void setChargeCurrent(std::uint16_t max_mA);

    /// set battery charge voltage
    /// @param max_mV milli volt. (4100 - 4360).
    void setChargeVoltage(std::uint16_t max_mV);

    /// @return -1:discharge / 0:standby / 1:charge
    int getChargeStatus(void);

    /// Get whether the battery is currently charging or not.
    bool isCharging(void);

    void powerOff(void);

    void setAdcState(bool enable);
    void setAdcRate( std::uint8_t rate );

    void setEXTEN(bool enable);
    void setBACKUP(bool enable);

    bool isACIN(void);
    bool isVBUS(void);
    bool getBatState(void);
    bool getEXTEN(void);

    float getBatteryVoltage(void);
    float getBatteryDischargeCurrent(void);
    float getBatteryChargeCurrent(void);
    float getBatteryPower(void);
    float getACINVoltage(void);
    float getACINCurrent(void);
    float getVBUSVoltage(void);
    float getVBUSCurrent(void);
    float getAPSVoltage(void);
    float getInternalTemperature(void);

    /// @return 0:none / 1:Long press / 2:Short press / 3:both
    std::uint8_t getPekPress(void);

    void setReg0x20Bit0(bool);

  private:
    std::size_t readRegister12(std::uint8_t addr);
    std::size_t readRegister14(std::uint8_t addr);
    std::size_t readRegister16(std::uint8_t addr);
  };
}

#endif
