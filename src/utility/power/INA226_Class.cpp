// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "INA226_Class.hpp"

#if __has_include(<esp_log.h>)
#include <esp_log.h>
#endif

#include <algorithm>

namespace m5
{
  bool INA226_Class::begin(void)
  {
    uint16_t id = readRegister16(0xFF);
#if defined (ESP_LOGV)
      // ESP_LOGV("INA226", "regFFh:%04x", id);
#endif
    _init = (id == 0x2260);
    return _init;
  }

  void INA226_Class::config(const config_t& cfg)
  {
    uint16_t value = (uint16_t)cfg.sampling_rate << 9
                   | (uint16_t)cfg.bus_conversion_time << 6
                   | (uint16_t)cfg.shunt_conversion_time << 3
                   | (uint16_t)cfg.mode;

    writeRegister16(INA226_CONFIG, value);

    float current_LSB = cfg.max_expected_current / 32768.0f;
    _cur_lsb = current_LSB;
    uint16_t cal = (uint16_t)(0.00512f / (current_LSB * cfg.shunt_res));
    _shunt_res = cfg.shunt_res;

    writeRegister16(INA226_CALIBRATION, cal);
  }

  float INA226_Class::getBusVoltage(void)
  {
    auto raw = (int16_t)readRegister16(INA226_BUS_V);
    return (raw * 0.00125f);
  }

  float INA226_Class::getShuntVoltage(void)
  {
    auto raw = (int16_t)readRegister16(INA226_SHUNT_V);
    return (raw * 0.0000025f);
  }

  float INA226_Class::getShuntCurrent(void)
  {
    auto raw = (int16_t)readRegister16(INA226_CURRENT);
    return (raw * _cur_lsb);
  }

  float INA226_Class::getPower(void)
  {
    auto raw = (int16_t)readRegister16(INA226_POWER);
    return (raw * _cur_lsb * 25.0f);
  }

  std::size_t INA226_Class::readRegister16(std::uint8_t addr)
  {
    std::uint8_t buf[2] = {0};
    readRegister(addr, buf, 2);
    return buf[0] << 8 | buf[1];
  }

  bool INA226_Class::writeRegister16(std::uint8_t addr, std::uint16_t data)
  {
    std::uint8_t buf[2] = {static_cast<std::uint8_t>(data >> 8), static_cast<std::uint8_t>(data & 0xFF)};
    return writeRegister(addr, buf, 2);
  }
}
