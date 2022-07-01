// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#if defined ( LGFX_SDL )

#else
#include "NeoPixel_Class.hpp"

#if __has_include (<esp_idf_version.h>)
 #include <esp_idf_version.h>
#endif
#include <esp_log.h>
#include <sdkconfig.h>
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S2)
 #define RMT_CH_TX_MAX (RMT_CHANNEL_MAX)
#else
 /// RMTチャネルの前半は送信用、後半は受信用 ( ESP32-C3, ESP32-S3 );
 #define RMT_CH_TX_MAX (RMT_CHANNEL_MAX >> 1)
#endif

#define NEOPIXEL_BIT_COUNT 24

namespace m5
{
  #define T0H 1  // 0 bit high time
  #define T0L 2  // 0 bit low time
  #define T1H 2  // 1 bit high time
  #define T1L 1  // 1 bit low time
  #define BLANKING_TIME_US 320

  static constexpr rmt_item32_t bit_tbl[2] = { {T0H, 1, T0L, 0}, {T1H, 1, T1L, 0} };

  static int last_rmt_ch = RMT_CH_TX_MAX;

  bool NeoPixel_Class::begin(gpio_num_t pin, size_t led_count, size_t freq, rmt_channel_t rmt_ch)
  {
    if (_rmt_buf) { m5gfx::heap_free(_rmt_buf); _rmt_buf = nullptr; }

    if (rmt_ch == (rmt_channel_t)-1)
    {
      if (last_rmt_ch <= 0) { return false; }
      auto ch = last_rmt_ch - 1;
      last_rmt_ch = ch;
      rmt_ch = rmt_channel_t(ch);
    }
    _rmt_ch = rmt_ch;

    auto basefreq = m5gfx::getApbFrequency();
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = _rmt_ch;
    config.gpio_num = pin;
    config.mem_block_num = 1;
    config.tx_config.loop_en = false;
    config.tx_config.carrier_en = false;
    config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = rmt_idle_level_t::RMT_IDLE_LEVEL_LOW;
    config.clk_div = (basefreq / (freq * (T0H+T0L) + 1)) + 1;

#if defined (ESP_IDF_VERSION_VAL)
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    config.flags = 0;   // フラグを0にし、APBクロックを使用する。(デフォルトはXTALクロックを使用);
  #endif
#endif

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    freq = basefreq / config.clk_div;
    _blanking_time = (uint64_t)(BLANKING_TIME_US-24) * freq / 1000000;

    _rmt_buf = (rmt_item32_t*)m5gfx::heap_alloc_dma(led_count * NEOPIXEL_BIT_COUNT * sizeof(rmt_item32_t));
    _led_count = led_count;

    return true;
  }

  void NeoPixel_Class::_rmt_send(const RGBColor& data, bool blanking)
  {
    uint32_t color = (((data.G8() * _brightness + 255) >> 8) << 16)
                   + (((data.R8() * _brightness + 255) >> 8) <<  8)
                   + (((data.B8() * _brightness + 255) >> 8)      );
    size_t i = 24;
    auto dst = _rmtbuffer;
    do
    {
      *dst++ = bit_tbl[1 & (color >> --i)];
    } while (i);
    if (blanking)
    {
      _rmtbuffer[NEOPIXEL_BIT_COUNT - 1].duration1 = _blanking_time;
    }
    rmt_write_items(_rmt_ch, _rmtbuffer, NEOPIXEL_BIT_COUNT, false);
  }

  void NeoPixel_Class::_rmt_set(size_t index, const RGBColor& data)
  {
    uint32_t color = (((data.G8() * _brightness + 255) >> 8) << 16)
                   + (((data.R8() * _brightness + 255) >> 8) <<  8)
                   + (((data.B8() * _brightness + 255) >> 8)      );
    size_t i = 24;
    auto dst = &_rmt_buf[index * NEOPIXEL_BIT_COUNT];
    do
    {
      *dst++ = bit_tbl[1 & (color >> --i)];
    } while (i);
  }

  void NeoPixel_Class::pushPixels(const RGBColor* data, size_t length)
  {
    if (_rmt_buf == nullptr) { return; }
    if (length > _led_count) { length = _led_count; }
    if (length == 0) { return; }
    do
    {
      _rmt_send(*data++, !length);
    } while (length--);
  }

  void NeoPixel_Class::pushImage(const lgfx::LGFX_Sprite* sprite)
  {
    if (_rmt_buf == nullptr) { return; }
    lgfx::pixelcopy_t pc(sprite->getBuffer(), lgfx::color_depth_t::rgb888_3Byte, sprite->getColorDepth());
    auto w = sprite->width();
    pc.src_width    = w;
    pc.src_bitwidth = w;
    if (pc.src_bits < 8)
    {
      uint32_t x_mask = (pc.src_bits == 1) ? 7
                      : (pc.src_bits == 2) ? 3
                                           : 1;
      pc.src_bitwidth = (w + x_mask) & (~x_mask);
    }

    size_t len = w * sprite->height();
    if (len > _led_count) { len = _led_count; }
    auto buf = (RGBColor*)alloca(len * sizeof(RGBColor));
    pc.fp_copy(buf, 0, len, &pc);
    pushPixels(buf, len);
  }

  void NeoPixel_Class::_clear(uint32_t bgr888)
  {
    if (_rmt_buf == nullptr) { return; }
    RGBColor col(bgr888);
    for (size_t i = 0; i < _led_count; ++i)
    {
      _rmt_set(i, col);
    }
    _rmt_buf[_led_count * NEOPIXEL_BIT_COUNT - 1].duration1 = _blanking_time;
    rmt_write_items(_rmt_ch, _rmt_buf, _led_count * NEOPIXEL_BIT_COUNT, false);
  }
}
#endif
