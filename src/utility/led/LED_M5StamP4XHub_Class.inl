// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_IMPLEMENTATION
#error "LED_M5StamP4XHub_Class.inl is part of M5Unified.cpp and is not meant to be included on its own"
#endif

#include "LED_M5StamP4XHub_Class.hpp"
#include "../../M5Unified.hpp"

#if defined (CONFIG_IDF_TARGET_ESP32P4)

namespace m5
{
// RGB LED is driven by the PM1:
//   Red   = LED_EN (PWR_CFG bit4, active high)
//   Green = GPIO4 (active low)
//   Blue  = GPIO0 (active low)

  bool LED_M5StamP4XHub_Class::begin(void)
  {
    auto& pmic = M5.Power.M5pm1;
    bool ok = pmic.setGPIOFunction(M5PM1_Class::gpio4, M5PM1_Class::gpio);
    ok = pmic.setGPIOFunction(M5PM1_Class::gpio0, M5PM1_Class::gpio) && ok;
    ok = pmic.setGPIODrive(M5PM1_Class::gpio4, M5PM1_Class::push_pull) && ok;
    ok = pmic.setGPIODrive(M5PM1_Class::gpio0, M5PM1_Class::push_pull) && ok;
    ok = pmic.setGPIOPull(M5PM1_Class::gpio4, M5PM1_Class::pull_none) && ok;
    ok = pmic.setGPIOPull(M5PM1_Class::gpio0, M5PM1_Class::pull_none) && ok;
    setBrightness(_brightness);
    // Active low: set the latches first so that a dark LED does not flash.
    ok = _apply() && ok;
    if (!ok) { return false; }
    ok = pmic.setGPIOMode(M5PM1_Class::gpio4, M5PM1_Class::output);
    return pmic.setGPIOMode(M5PM1_Class::gpio0, M5PM1_Class::output) && ok;
  }

  void LED_M5StamP4XHub_Class::setColors(const RGBColor* values, size_t index, size_t length)
  {
    if (index >= led_count) {
      return;
    }
    if (length > led_count - index) {
      length = led_count - index;
    }
    std::copy(values, values + length, &_rgb_buffer + index);
  }

  void LED_M5StamP4XHub_Class::setBrightness(const uint8_t brightness)
  {
    _brightness = brightness;
  }

  void LED_M5StamP4XHub_Class::display(void)
  {
    _apply();
  }

  bool LED_M5StamP4XHub_Class::_apply(void)
  {
    uint32_t br = _brightness + 1;
    br = br * br;

    const uint16_t r = (_rgb_buffer.R8() * br) >> 8;
    const uint16_t g = (_rgb_buffer.G8() * br) >> 8;
    const uint16_t b = (_rgb_buffer.B8() * br) >> 8;

    auto& pmic = M5.Power.M5pm1;
    bool ok = pmic.setLedEnLevel(r >= 2048);
    // Green and blue are active low: a lit LED needs a low output level.
    ok = pmic.setGPIOOutput(M5PM1_Class::gpio4, g < 2048) && ok;
    return pmic.setGPIOOutput(M5PM1_Class::gpio0, b < 2048) && ok;
  }
}

#endif
