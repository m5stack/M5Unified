// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef M5_POWERHUB_CLASS_H__
#define M5_POWERHUB_CLASS_H__

#include "LED_Base.hpp"
#include "../I2C_Class.hpp"
#include <vector>

namespace m5
{
  class LED_PowerHub_Class : public LED_Base, public I2C_Device
  {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x50;
    LED_PowerHub_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
      : I2C_Device(i2c_addr, freq, i2c) {}

    bool begin(void) override;
    led_type_t getLedType(size_t index) const override { return led_type_t::led_type_fullcolor; }
    size_t getCount(void) const override { return 8; }
    void setColors(const RGBColor* values, size_t index, size_t length) override;
    void setBrightness(const uint8_t brightness) override;
    void display(void) override;
    RGBColor* getBuffer(void) override { return _rgb_buffer.data(); }

  private:
    std::vector<RGBColor> _rgb_buffer;
    uint8_t _brightness = 63;
  };
}

#endif
