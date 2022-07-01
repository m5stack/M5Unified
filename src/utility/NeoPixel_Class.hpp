// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_NeoPixel_Class_H__
#define __M5_NeoPixel_Class_H__

#include <M5GFX.h>

#include <driver/rmt.h>

namespace m5
{
  class NeoPixel_Class
  {
  public:
    static inline constexpr uint8_t  color332(uint8_t r, uint8_t g, uint8_t b) { return m5gfx::color332(r, g, b); }
    static inline constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return m5gfx::color565(r, g, b); }
    static inline constexpr uint32_t color888(uint8_t r, uint8_t g, uint8_t b) { return m5gfx::color888(r, g, b); }

    bool isEnabled(void) const { return _rmt_buf != nullptr; }

    bool begin(gpio_num_t pin, size_t led_count = 1, size_t freq = 833334, rmt_channel_t rmt_ch = rmt_channel_t(-1));

    void setBrightness(uint8_t brightness) { _brightness = brightness; }

    void pushPixels(const RGBColor* colors, size_t length);

    void pushImage(const lgfx::LGFX_Sprite* sprite);

    template <typename T>
    void clear(const T& color) { _clear(_color_conv.convert(color)); };

  protected:
    rmt_item32_t _rmtbuffer[24];

    void _clear(uint32_t bgr888);

    m5gfx::color_conv_t _color_conv = { m5gfx::color_depth_t::rgb888_3Byte };

    size_t _led_count = 0;
    rmt_item32_t* _rmt_buf = nullptr;
    rmt_channel_t _rmt_ch = rmt_channel_t(-1);
    uint16_t _blanking_time = 256;
    uint8_t _brightness = 16;

    void _rmt_set(size_t index, const RGBColor& data);
    void _rmt_send(const RGBColor& data, bool blanking);
  };
}
#endif
