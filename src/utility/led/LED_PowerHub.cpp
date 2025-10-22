// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "LED_PowerHub.hpp"
#include "esp_log.h"
#include "Arduino.h"

namespace m5
{
  constexpr size_t led_count = 8;
  bool LED_PowerHub::begin(void)
  {
    if (!_bus)
    {
      return false;
    }

    if (!_bus->begin())
    {
      return false;
    }
    _rgb_buffer.resize(led_count);
    writeRegister8(0x00, 1); // Enable Led Power

    return true;
  }

  void LED_PowerHub::setColors(const m5gfx::rgb888_t* values, size_t index, size_t length)
  {
      if (index + length > led_count) {
          length = led_count - index;
      }
    std::copy(values, values + length, _rgb_buffer.begin() + index);
  }
  void LED_PowerHub::setBrightness(const uint8_t brightness)
  {
    _brightness = brightness;
  }
  void LED_PowerHub::display(void)
  {
    std::array<uint8_t, 32> send_buffer;
    std::array<uint8_t, 8> br_buffer;
    
    br_buffer.fill(_brightness);
    
    int b_idx = 0;
    int g_idx = 1;
    int r_idx = 2;

    auto dst = send_buffer.data();
    for (size_t i = 0; i < led_count; i++)
    {
      uint8_t r = _rgb_buffer[i].R8();
      uint8_t g = _rgb_buffer[i].G8();
      uint8_t b = _rgb_buffer[i].B8();
    
      dst[b_idx] = b;
      dst[g_idx] = g;
      dst[r_idx] = r;
      dst[3] = 0x00;
      dst += 4;

    }
    writeRegister(0x60, send_buffer.data(), send_buffer.size());
    writeRegister(0x80, br_buffer.data(), br_buffer.size());
  }
}