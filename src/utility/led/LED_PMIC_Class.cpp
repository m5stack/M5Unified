// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "LED_PMIC_Class.hpp"

namespace m5
{
  bool LED_PMIC_Class::begin(void)
  {
    if (!bitOn(0x06, 1 << 4)) // Enable Led Power
    {
      return false;
    }
    
    uint8_t buf = readRegister8(0x50);
    buf = (buf & 0xE0) | _config.led_count;
    if (!writeRegister8(0x50, buf)) {
        return false;
    }

    if (0 <= _config.pin_data && _config.pin_data < 5)
    {
      uint8_t pin = (_config.pin_data);
      uint8_t shift = ((pin & 0x03) << 1);
      uint8_t reg_fun = (pin < 4) ? 0x16 : 0x17;  // GPIO_FUNC0 / GPIO_FUNC1

      uint8_t reg_val = readRegister8(reg_fun);
      reg_val = ((reg_val & ~(0x03u << shift)) | (0x03u << shift)); // 2-bit func = 11 (mux)

      if (!writeRegister8(reg_fun, reg_val)) {
        return false;
      }
      bitOff(0x13, 1 << pin);
    }

    setBrightness(_brightness);
    _rgb_buffer.resize(_config.led_count);
    _send_buffer.resize(_config.led_count * 2);
    return true;
  }

  void LED_PMIC_Class::setColors(const RGBColor* values, size_t index, size_t length)
  {
    if (index + length > _config.led_count) {
      length = _config.led_count - index;
    }
    std::copy(values, values + length, _rgb_buffer.begin() + index);
  }

  void LED_PMIC_Class::setBrightness(const uint8_t brightness)
  {
    _brightness = brightness;
  }

  void LED_PMIC_Class::display(void)
  {
    uint32_t br = _brightness + 1;
    br = br * br;

    auto dst = _send_buffer.data();
    for (size_t i = 0; i < _config.led_count; i++)
    {
      uint8_t r = _rgb_buffer[i].R8();
      uint8_t g = _rgb_buffer[i].G8();
      uint8_t b = _rgb_buffer[i].B8();
      r = (r * br) >> 16;
      g = (g * br) >> 16;
      b = (b * br) >> 16;

      uint16_t color565 = lgfx::color565(r, g, b);
      dst[0] = color565 & 0xFF;
      dst[1] = color565 >> 8;
      dst += 2;
    }
    writeRegister(0x60, _send_buffer.data(), _send_buffer.size());
    bitOn(0x50, 1 << 6); // Update Led Color
  }
}