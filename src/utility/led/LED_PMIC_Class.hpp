// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef M5_PMIC_CLASS_H__
#define M5_PMIC_CLASS_H__

#include "LED_Base.hpp"
#include "../I2C_Class.hpp"
#include <vector>

namespace m5
{
  class LED_PMIC_Class : public LED_Base, public I2C_Device
  {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x6E;
    //Default I2C frequency 100K
    LED_PMIC_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 100000, I2C_Class* i2c = &In_I2C)
      : I2C_Device(i2c_addr, freq, i2c) {}
    
    struct config_t {
      size_t led_count = 1;
      int8_t pin_data = 1;
    };
    const config_t& config(void) const { return _config; }
    const config_t& getConfig(void) const { return _config; }
    void config(const config_t& cfg) { setConfig(cfg); }
    void setConfig(const config_t& cfg) { _config = cfg; }

    bool begin(void) override;
    led_type_t getLedType(size_t index) const override { return led_type_t::led_type_fullcolor; }
    size_t getCount(void) const override { return 8; }
    void setColors(const RGBColor* values, size_t index, size_t length) override;
    void setBrightness(const uint8_t brightness) override;
    void display(void) override;
    RGBColor* getBuffer(void) override { return _rgb_buffer.data(); }

  private:
    config_t _config;
    std::vector<RGBColor> _rgb_buffer;
    std::vector<uint8_t> _send_buffer;
    uint8_t _brightness = 63;
  };
}

#endif
