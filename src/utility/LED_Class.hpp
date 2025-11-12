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
    RGBColor* getBuffer(void);

    void setBrightness(uint8_t brightness);

    void setAutoDisplay(bool enable) { _auto_display = enable; }

    void setAllColor(const RGBColor& color);
    void setAllColor(uint8_t red, uint8_t green, uint8_t blue) { setAllColor((RGBColor){ red, green, blue }); }

    template <typename T>
    void setAllColor(T c) { const RGBColor color = m5gfx::convert_to_bgr888(c); setAllColor(color); }

    void setColor(size_t index, const RGBColor& color);
    void setColor(size_t index, uint8_t red, uint8_t green, uint8_t blue) { setColor(index, (RGBColor){ red, green, blue }); }

    template <typename T>
    void setColor(size_t index, T c) { const RGBColor color = m5gfx::convert_to_bgr888(c); setColor(index, color); }

    template <typename T>
    void setColors(const T* values, size_t index = 0, size_t length = INT32_MAX)
    {
      auto pc = create_pc_fast(values);
      _set_colors(index, length, &pc);
    }

    LED_Base::led_type_t getLedType(size_t index) const;
    bool isEnabled(void) const { return _led_instance.get() != nullptr; }
    LED_Base* getLedInstancePtr(void) const { return _led_instance.get(); }
    void setLedInstance(std::shared_ptr<LED_Base> instance) { _led_instance = instance; }

  private:
    // instance holder
    std::shared_ptr<LED_Base> _led_instance;
    bool _initialized = false;
    bool _auto_display = true;
    bool _swapBytes = false;
    void _set_colors(size_t index, size_t length, lgfx::pixelcopy_t* pc);

    template<typename T>
    lgfx::pixelcopy_t create_pc_fast(const T *data)
    {
      lgfx::pixelcopy_t pc(data, RGBColor::depth, lgfx::get_depth<T>::value, false);
      pc.fp_copy = lgfx::pixelcopy_t::copy_rgb_fast<RGBColor, T>;
      return pc;
    }
    lgfx::pixelcopy_t create_pc_fast(const uint8_t*  data) { return create_pc_fast(reinterpret_cast<const lgfx::rgb332_t*>(data)); }
    lgfx::pixelcopy_t create_pc_fast(const uint16_t* data) { return create_pc_fast(data, _swapBytes); }
    lgfx::pixelcopy_t create_pc_fast(const void*     data) { return create_pc_fast(data, _swapBytes); }
    lgfx::pixelcopy_t create_pc_fast(const uint16_t* data, bool swap)
    {
      return swap
           ? create_pc_fast(reinterpret_cast<const lgfx::rgb565_t* >(data))
           : create_pc_fast(reinterpret_cast<const lgfx::swap565_t*>(data));
    }
    lgfx::pixelcopy_t create_pc_fast(const void *data, bool swap)
    {
      return swap
           ? create_pc_fast(reinterpret_cast<const lgfx::rgb888_t*>(data))
           : create_pc_fast(reinterpret_cast<const lgfx::bgr888_t*>(data));
    }
  };
}

#endif
