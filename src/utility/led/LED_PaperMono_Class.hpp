// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef M5_PAPERMONO_CLASS_H__
#define M5_PAPERMONO_CLASS_H__

#include "LED_Base.hpp"
#include "../I2C_Class.hpp"

namespace m5
{
  class LED_PaperMono_Class : public LED_Base
  {
  public:
    LED_PaperMono_Class() {}

    bool begin(void) override;
    led_type_t getLedType(size_t index) const override { return led_type_t::led_type_fullcolor; }
    size_t getCount(void) const override { return led_count; }
    void setColors(const RGBColor* values, size_t index, size_t length) override;
    void setBrightness(const uint8_t brightness) override;
    void display(void) override;
    RGBColor* getBuffer(void) override { return &_rgb_buffer; }

  private:
    static constexpr size_t led_count = 1;
    RGBColor _rgb_buffer;
    uint8_t _brightness = 63;
  };
}

#endif
