// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "BQ27220_Class.hpp"

#include <M5Unified.h>

#include <algorithm>
#include <math.h>

namespace m5
{
  bool BQ27220_Class::read_MuxAddrdata(uint8_t regAddr, uint16_t subReg, uint8_t *regData, uint8_t length)
  {
    if (_i2c->start(_addr, false, _freq)) {
      if (subReg != 0) {
          _i2c->write(subReg & 0xFF);
          _i2c->write((subReg >> 8) & 0xFF);
      }
      _i2c->stop();
      if (length == 0) {
        return true;
      }
      M5.delay(10);
      if (_i2c->readRegister(_addr, 0x3E, regData, length, _freq)) {
        return true;
      }
    }
    return false;
  } 

  bool BQ27220_Class::begin(void)
  {
    // reset
    _init = read_MuxAddrdata(0x00, 0x0041);
    if (_init) {
      uint8_t read_data[4] = {0};
      read_MuxAddrdata(0x00, 0x0001, read_data, 4);
  // printf("BQ27220 W:0x00->0x0001 R:0x%02X %02X %02X %02X\r\n", read_data[0], read_data[1], read_data[2], read_data[3]);

      M5.delay(200);

      //  exit_sealed
      read_MuxAddrdata(0x00, 0x8000);
      read_MuxAddrdata(0x00, 0x8000);
  
      // full_access
      read_MuxAddrdata(0x00, 0xFFFF);
      read_MuxAddrdata(0x00, 0xFFFF);
    }

    return _init;
  }

  int16_t BQ27220_Class::getCurrent_mA(void)
  {
    uint8_t buf[2] = {0};
    if (_i2c->readRegister(_addr, 0x14, buf, 2, _freq)) {
      int16_t current_mA = (buf[0] | (buf[1] << 8));
      return current_mA;
    }
    return 0; // Return 0 if read failed
  }

  float BQ27220_Class::getCurrent_F(void)
  {
    uint8_t buf[2] = {0};
    if (_i2c->readRegister(_addr, 0x14, buf, 2, _freq)) {
      int16_t current = (buf[0] | (buf[1] << 8));
      return current / 1000.0f;
    }
    return nanf(""); // Return NaN if read failed
  }

  int16_t BQ27220_Class::getVoltage_mV(void)
  {
    uint8_t buf[2] = {0};
    if (_i2c->readRegister(_addr, 0x08, buf, 2, _freq)) {
      uint16_t voltage_mV = (buf[0] | (buf[1] << 8));
      return voltage_mV / 1000.0f; // Convert to volts
    }
    return 0; // Return 0 if read failed
  }

  float BQ27220_Class::getVoltage_F(void)
  {
    uint8_t buf[2] = {0};
    if (_i2c->readRegister(_addr, 0x08, buf, 2, _freq)) {
      uint16_t voltage_mV = (buf[0] | (buf[1] << 8));
      return voltage_mV / 1000.0f; // Convert to volts
    }
    return nanf(""); // Return NaN if read failed
  }
}
