// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef M5_LED_CLASS_H__
#define M5_LED_CLASS_H__

#include "led/LED_Base.hpp"

#include <memory>

namespace m5
{
  class LED_Class
  {
  public:

    bool begin(void);
    void display(void);

    size_t getCount(void) const;
    void setBrightness(uint8_t brightness);

    void setAllColor(const m5gfx::rgb888_t& rgb888);
    void setAllColor(uint8_t red, uint8_t green, uint8_t blue) { setAllColor((m5gfx::rgb888_t){ red, green, blue }); }
    void setAllColor(uint32_t rgb888) { setAllColor((m5gfx::rgb888_t){ rgb888 }); }
    void setAllColor(uint16_t rgb565) { setAllColor(m5gfx::convert_to_rgb888(rgb565)); }
    void setAllColor(int rgb565) { setAllColor(m5gfx::convert_to_rgb888(rgb565)); }
    void setAllColor(uint8_t rgb332) { setAllColor(m5gfx::convert_to_rgb888(rgb332)); }

    void setColor(size_t index, const m5gfx::rgb888_t& rgb888);
    void setColor(size_t index, uint8_t red, uint8_t green, uint8_t blue) { setColor(index, (m5gfx::rgb888_t){ red, green, blue }); }
    void setColor(size_t index, uint32_t rgb888) { setColor(index, (m5gfx::rgb888_t){ rgb888 }); }
    void setColor(size_t index, uint16_t rgb565) { setColor(index, m5gfx::convert_to_rgb888(rgb565)); }
    void setColor(size_t index, int rgb565) { setColor(index, m5gfx::convert_to_rgb888(rgb565)); }
    void setColor(size_t index, uint8_t rgb332) { setColor(index, m5gfx::convert_to_rgb888(rgb332)); }

    void setLedColors(const m5gfx::rgb888_t* values, size_t index, size_t length);
    
    LED_Base::led_type_t getLedType(size_t index) const;
    bool isEnabled(void) const { return _led_instance.get() != nullptr; }
    LED_Base* getLedInstancePtr(void) const { return _led_instance.get(); }
    void setLedInstance(std::shared_ptr<LED_Base> instance) { _led_instance = instance; }

  private:
    // instance holder
    std::shared_ptr<LED_Base> _led_instance;
    bool _initialized = false;
  };
}

#endif
