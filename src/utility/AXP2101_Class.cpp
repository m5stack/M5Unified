// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "AXP2101_Class.hpp"

#if defined ( ARDUINO )
#include <Arduino.h>
#endif

#include <esp_log.h>
#include <algorithm>

namespace m5
{
/*
  DCDC1 : 1.5-3.4V，                      2000mA
  DCDC2 : 0.5-1.2V，1.22-1.54V,           2000mA
  DCDC3 : 0.5-1.2V，1.22-1.54V, 1.6-3.4V, 2000mA
  DCDC4 : 0.5-1.2V, 1.22-1.84V,           1500mA
  DCDC5 : 1.2V    , 1.4-3.7V,             1000mA

  RTCLDO1/2 : 1.8V/2.5V/3V/3.3V,            30mA

  ALDO1~4 : 0.5-3.5V, 100mV/step            300mA
*/
  bool AXP2101_Class::begin(void)
  {
    std::uint8_t val;
    _init = readRegister(0x03, &val, 1);
    if (_init)
    {
      ESP_LOGV("AXP2101", "reg03h:%02x", val);
      _init = (val == 0x4A);
    }
    return _init;
  }

  void AXP2101_Class::setReg0x20Bit0(bool flg)
  {
ESP_LOGE("AXP2101","setReg0x20Bit0 : %d", flg);
    writeRegister8(AXP2101_EFUS_OP_CFG, 0x06);
    bitOn(AXP2101_EFREQ_CTRL, 0x04);
    writeRegister8(AXP2101_TWI_ADDR_EXT, 0x01);
    if (flg)
    {
      bitOn(0x20, 1);
    }
    else
    {
      bitOff(0x20, 1);
    }
    writeRegister8(AXP2101_TWI_ADDR_EXT, 0x00);
    bitOff(AXP2101_EFREQ_CTRL, 0x04);
    writeRegister8(AXP2101_EFUS_OP_CFG, 0x00);
  }

  void AXP2101_Class::setBatteryCharge(bool enable)
  {
    std::uint8_t val = 0;
    if (readRegister(0x18, &val, 1))
    {
      writeRegister8(0x18, (val & 0xFD) + (enable << 1));
    }
  }

  void AXP2101_Class::setChargeCurrent(std::uint16_t max_mA)
  {
  }

  void AXP2101_Class::setChargeVoltage(std::uint16_t max_mV)
  {
  }

  std::int8_t AXP2101_Class::getBatteryLevel(void)
  {
    std::int8_t res = readRegister8(0xA4);
    return res;
  }

  /// @return -1:discharge / 0:standby / 1:charge
  int AXP2101_Class::getChargeStatus(void)
  {
    uint32_t val = (readRegister8(0x01) >> 5) & 0b11;
    // 0b01:charge / 0b10:dischage / 0b00:stanby
    return (val == 1) ? 1 : ((val == 2) ? -1 : 0);
  }

  bool AXP2101_Class::isCharging(void)
  {
    return (readRegister8(0x01) & 0b01100000) == 0b00100000;
  }

  void AXP2101_Class::powerOff(void)
  {
  }

  void AXP2101_Class::setAdcState(bool enable)
  {
  }

  void AXP2101_Class::setAdcRate( std::uint8_t rate )
  {
  }

  void AXP2101_Class::setEXTEN(bool enable)
  {
/*
    static constexpr std::uint8_t add = 0x12;
    static constexpr std::uint8_t bit = 1 << 6;
    if (enable)
    {
      bitOn(add, bit);
    }
    else
    {
      bitOff(add, bit);
    }
//*/
  }

  bool AXP2101_Class::getEXTEN(void)
  {
return 0;
  }

  void AXP2101_Class::setBACKUP(bool enable)
  {
/*
    static constexpr std::uint8_t add = 0x35;
    static constexpr std::uint8_t bit = 1 << 7;
    if (enable)
    { // Enable
      bitOn(add, bit);
    }
    else
    { // Disable
      bitOff(add, bit);
    }
//*/
  }

  bool AXP2101_Class::isACIN(void)
  {
return false;
  }
  bool AXP2101_Class::isVBUS(void)
  {
return false;
  }

  bool AXP2101_Class::getBatState(void)
  {
return false;
  }

  std::uint8_t AXP2101_Class::getPekPress(void)
  {
    std::uint8_t val = readRegister8(0x49) & 0x0C;
    if (val) { writeRegister8(0x49, val); }
    return val >> 2;
  }

  float AXP2101_Class::getACINVoltage(void)
  {
return 0;
  }

  float AXP2101_Class::getACINCurrent(void)
  {
return 0;
  }

  float AXP2101_Class::getVBUSVoltage(void)
  {
return 0;
  }

  float AXP2101_Class::getVBUSCurrent(void)
  {
return 0;
  }

  float AXP2101_Class::getInternalTemperature(void)
  {
return 0;
  }

  float AXP2101_Class::getBatteryPower(void)
  {
return 0;
  }

  float AXP2101_Class::getBatteryVoltage(void)
  {
return 0;
  }

  float AXP2101_Class::getBatteryChargeCurrent(void)
  {
return 0;
  }

  float AXP2101_Class::getBatteryDischargeCurrent(void)
  {
return 0;
  }

  float AXP2101_Class::getAPSVoltage(void)
  {
return 0;
  }


  std::size_t AXP2101_Class::readRegister12(std::uint8_t addr)
  {
    std::uint8_t buf[2] = {0};
    readRegister(addr, buf, 2);
    return buf[0] << 4 | buf[1];
  }
  std::size_t AXP2101_Class::readRegister13(std::uint8_t addr)
  {
    std::uint8_t buf[2] = {0};
    readRegister(addr, buf, 2);
    return buf[0] << 5 | buf[1];
  }
  std::size_t AXP2101_Class::readRegister16(std::uint8_t addr)
  {
    std::uint8_t buf[2] = {0};
    readRegister(addr, buf, 2);
    return buf[0] << 8 | buf[1];
  }
  std::size_t AXP2101_Class::readRegister24(std::uint8_t addr)
  {
    std::uint8_t buf[3] = {0};
    readRegister(addr, buf, 3);
    return buf[0] << 16 | buf[1] << 8 | buf[2];
  }
  std::size_t AXP2101_Class::readRegister32(std::uint8_t addr)
  {
    std::uint8_t buf[4] = {0};
    readRegister(addr, buf, 4);
    return buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
  }
}
