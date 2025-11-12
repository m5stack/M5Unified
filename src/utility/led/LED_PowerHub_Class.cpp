// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "LED_PowerHub_Class.hpp"

#if defined (CONFIG_IDF_TARGET_ESP32S3)

namespace m5
{
  constexpr size_t led_count = 8;
  bool LED_PowerHub_Class::begin(void)
  {
    _rgb_buffer.resize(led_count);
    if (writeRegister8(0x00, 1)) // Enable Led Power
    {
      setBrightness(_brightness);
      return true;
    }
    return false;
  }

  void LED_PowerHub_Class::setColors(const RGBColor* values, size_t index, size_t length)
  {
    if (index + length > led_count) {
      length = led_count - index;
    }
    std::copy(values, values + length, _rgb_buffer.begin() + index);
  }

  void LED_PowerHub_Class::setBrightness(const uint8_t brightness)
  {
    _brightness = brightness;
    std::array<uint8_t, led_count> br_buffer;
    br_buffer.fill(brightness);
    writeRegister(0x80, br_buffer.data(), br_buffer.size());
  }

  void LED_PowerHub_Class::display(void)
  {
    std::array<uint8_t, led_count * 4> send_buffer;

    auto dst = send_buffer.data();
    for (size_t i = 0; i < led_count; i++)
    {
      dst[0] = _rgb_buffer[i].B8();
      dst[1] = _rgb_buffer[i].G8();
      dst[2] = _rgb_buffer[i].R8();
      dst[3] = 0x00;
      dst += 4;
    }
    writeRegister(0x60, send_buffer.data(), send_buffer.size());
  }
}

#endif
