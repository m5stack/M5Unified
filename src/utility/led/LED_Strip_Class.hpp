// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef M5_LED_STRIP_CLASS_H__
#define M5_LED_STRIP_CLASS_H__

#include "LED_Base.hpp"

#include <vector>
#include <memory>

#if __has_include (<sdkconfig.h>)
 #include <sdkconfig.h>
 #include <esp_idf_version.h>
// RMT
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #define M5UNIFIED_RMT_VERSION 2
 #else
  #define M5UNIFIED_RMT_VERSION 1
 #endif
#endif

#if M5UNIFIED_RMT_VERSION == 2
 #include <driver/rmt_types.h>
 #include <driver/rmt_tx.h>
#elif M5UNIFIED_RMT_VERSION == 1
 #include <driver/rmt.h>
 #include <soc/rmt_struct.h>
#endif

namespace m5
{
  class LedBus_Base;

//----------------------------------------------------------------------------

  class LED_Strip_Class : public LED_Base
  {
  public:
    LED_Strip_Class() {}

    struct config_t {
      enum color_order_t {
        color_order_rgb,
        color_order_rbg,
        color_order_grb,
        color_order_gbr,
        color_order_brg,
        color_order_bgr,
      };

      size_t led_count = 1;
      color_order_t color_order = color_order_grb;
      uint8_t byte_per_led = 3;
    };

    const config_t& config(void) const { return _config; }
    const config_t& getConfig(void) const { return _config; }
    void config(const config_t& cfg) { setConfig(cfg); }
    void setConfig(const config_t& cfg) { _config = cfg; }
    void setBus(std::shared_ptr<LedBus_Base> bus) { _bus = bus; }

    bool begin(void) override;
    led_type_t getLedType(size_t index) const override { return led_type_t::led_type_fullcolor; }
    size_t getCount(void) const override { return _config.led_count; }
    void setColors(const RGBColor* values, size_t index, size_t length) override;
    void setBrightness(const uint8_t brightness) override;
    void display(void) override;
    RGBColor* getBuffer(void) override { return _rgb_buffer.data(); }

  private:
    std::shared_ptr<LedBus_Base> _bus;
    std::vector<RGBColor> _rgb_buffer;
    std::vector<uint8_t> _send_buffer;
    config_t _config;
    uint8_t _brightness = 63;
  };

//----------------------------------------------------------------------------

  class LedBus_Base
  {
  public:
    virtual ~LedBus_Base() {}
    virtual bool init(void) = 0;
    virtual void release(void) = 0;
    virtual void write(const uint8_t* data, size_t length) = 0;
  };

  class LedBus_RMT : public LedBus_Base
  {
  public:
    LedBus_RMT() {}
    struct config_t {
      uint32_t frequency = 10000000; // 10MHz
      uint16_t t0h_ns = 300;
      uint16_t t0l_ns = 900;
      uint16_t t1h_ns = 900;
      uint16_t t1l_ns = 300;
      uint16_t reset_us = 280;
      int8_t pin_data = -1;
    };
    const config_t& config(void) const { return _config; }
    const config_t& getConfig(void) const { return _config; }
    void config(const config_t& cfg) { setConfig(cfg); }
    void setConfig(const config_t& cfg) { _config = cfg; }
    bool init(void) override;
    void release(void) override;
    void write(const uint8_t* data, size_t length) override;
  private:
    config_t _config;

#if M5UNIFIED_RMT_VERSION == 2
    rmt_channel_handle_t _rmt_ch_handle = nullptr;
    rmt_encoder_handle_t _led_encoder = nullptr;
#elif M5UNIFIED_RMT_VERSION == 1
#endif
  };

//----------------------------------------------------------------------------
}

#endif
