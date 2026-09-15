// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_I2C_ADDR_HPP_
#define M5UNIFIED_I2C_ADDR_HPP_

#include <stdint.h>

// I2C addresses of on-board devices that more than one implementation file talks to.
// (Internal use. Kept in one place so the single-translation-unit build has one definition.)
namespace m5
{
  static constexpr uint32_t i2c_freq = 100000;

  static constexpr uint8_t m5pm1_i2c_addr    = 0x6E;
  static constexpr uint8_t aw9523_i2c_addr   = 0x58;
  static constexpr uint8_t powerhub_i2c_addr = 0x50;
}

#endif
