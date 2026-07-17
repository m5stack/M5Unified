// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "LED_PaperMono_Class.hpp"
#include "../../M5Unified.hpp"
#include "../M5IOE1_Class.hpp"

#if defined (CONFIG_IDF_TARGET_ESP32S3)

namespace m5
{
// RGBLED_R = PMIC -> LED_EN_PP
// RGBLED_G = PYB -> G8
// RGBLED_B = PYB -> G9

  static constexpr size_t led_count = 1;
  static constexpr uint8_t m5pm1_i2c_addr = 0x6E;
  static constexpr uint32_t i2c_freq = 100000;
  static constexpr auto ioe1_led_g_pin = M5IOE1_Class::gpio8;
  static constexpr auto ioe1_led_b_pin = M5IOE1_Class::gpio9;

  bool LED_PaperMono_Class::begin(void)
  {
    // PM1 LED_EN (red), set PP mode and enable LED output
    M5.In_I2C.bitOff(m5pm1_i2c_addr, 0x13, 0x20, i2c_freq);

    auto& ioe1 = static_cast<M5IOE1_Class&>(M5.getIOExpander(0));
    ioe1.setDirection(ioe1_led_g_pin, true);
    ioe1.setDirection(ioe1_led_b_pin, true);
    ioe1.setHighImpedance(ioe1_led_g_pin, false);
    ioe1.setHighImpedance(ioe1_led_b_pin, false);

    setBrightness(_brightness);
    return true;
  }

  void LED_PaperMono_Class::setColors(const RGBColor* values, size_t index, size_t length)
  {
    if (index >= led_count) {
      return;
    }
    if (length > led_count - index) {
      length = led_count - index;
    }
    std::copy(values, values + length, &_rgb_buffer + index);
  }

  void LED_PaperMono_Class::setBrightness(const uint8_t brightness)
  {
    _brightness = brightness;
  }

  void LED_PaperMono_Class::display(void)
  {
    // RED = PMIC -> LED_EN_PP
    // GREEN = PYB -> G8
    // BLUE = PYB -> G9
    uint32_t br = _brightness + 1;
    br = br * br;

    uint16_t r = _rgb_buffer.R8();
    uint16_t g = _rgb_buffer.G8();
    uint16_t b = _rgb_buffer.B8();
    r = (r * br) >> 8;
    g = (g * br) >> 8;
    b = (b * br) >> 8;

    if (r < 2048) {
      M5.In_I2C.bitOff(m5pm1_i2c_addr, 0x06, 0x10, i2c_freq);
    } else {
      M5.In_I2C.bitOn(m5pm1_i2c_addr, 0x06, 0x10, i2c_freq);
    }

    auto& ioe1 = static_cast<M5IOE1_Class&>(M5.getIOExpander(0));
    ioe1.digitalWrite(ioe1_led_g_pin, g >= 2048);
    ioe1.digitalWrite(ioe1_led_b_pin, b >= 2048);

    if (g > 4095) { g = 4095; }
    ioe1.setPwmDuty(M5IOE1_Class::pwm_ch2, g, g > 0);
  }
}

#endif
