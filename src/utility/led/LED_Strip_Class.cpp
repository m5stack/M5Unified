// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "LED_Strip_Class.hpp"

#if M5UNIFIED_RMT_VERSION == 2

extern "C" {
  typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
  } rmt_led_strip_encoder_t;

  typedef struct {
    uint16_t t0h_tick;
    uint16_t t0l_tick;
    uint16_t t1h_tick;
    uint16_t t1l_tick;
    uint16_t reset_tick;
  } led_strip_encoder_config_t;

  static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
  {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    switch (led_encoder->state) {
    case 0: // send RGB data
      encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
      if (session_state & RMT_ENCODING_COMPLETE) {
        led_encoder->state = 1; // switch to next state when current encoding session finished
      }
      if (session_state & RMT_ENCODING_MEM_FULL) {
        state = (rmt_encode_state_t)(state | RMT_ENCODING_MEM_FULL);
        goto out;
      }
    // fall-through
    case 1: // send reset code
      encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                              sizeof(led_encoder->reset_code), &session_state);
      if (session_state & RMT_ENCODING_COMPLETE) {
        led_encoder->state = RMT_ENCODING_RESET;
        state = (rmt_encode_state_t)(state | RMT_ENCODING_COMPLETE);
      }
      if (session_state & RMT_ENCODING_MEM_FULL) {
        state = (rmt_encode_state_t)(state | RMT_ENCODING_MEM_FULL);
        goto out;
      }
    }
out:
    *ret_state = state;
    return encoded_symbols;
  }

  static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
  {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
   return ESP_OK;
  }
  static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
  {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
  }

  static esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
  {
    rmt_led_strip_encoder_t *led_encoder = NULL;
    if (config && ret_encoder) {
      led_encoder = (rmt_led_strip_encoder_t*)rmt_alloc_encoder_mem(sizeof(rmt_led_strip_encoder_t));
      if (led_encoder) {
        led_encoder->base.encode = rmt_encode_led_strip;
        led_encoder->base.del = rmt_del_led_strip_encoder;
        led_encoder->base.reset = rmt_led_strip_encoder_reset;
        rmt_bytes_encoder_config_t bytes_encoder_config;
        bytes_encoder_config.bit0.level0 = 1;
        bytes_encoder_config.bit0.duration0 = config->t0h_tick;
        bytes_encoder_config.bit0.level1 = 0;
        bytes_encoder_config.bit0.duration1 = config->t0l_tick;
        bytes_encoder_config.bit1.level0 = 1;
        bytes_encoder_config.bit1.duration0 = config->t1h_tick;
        bytes_encoder_config.bit1.level1 = 0;
        bytes_encoder_config.bit1.duration1 = config->t1l_tick;
        bytes_encoder_config.flags.msb_first = 1;
        if (ESP_OK == rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder)) {
          rmt_copy_encoder_config_t copy_encoder_config = {};
          if (ESP_OK == rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder)) {
            uint32_t reset_ticks = config->reset_tick >> 1;
            led_encoder->reset_code.level0 = 0;
            led_encoder->reset_code.duration0 = reset_ticks;
            led_encoder->reset_code.level1 = 0;
            led_encoder->reset_code.duration1 = config->reset_tick - reset_ticks;
            *ret_encoder = &led_encoder->base;
            return ESP_OK;
          }
          rmt_del_encoder(led_encoder->bytes_encoder);
        }
        free(led_encoder);
      }
    }
    return ESP_FAIL;
  }
}


#elif M5UNIFIED_RMT_VERSION == 1
#endif

namespace m5
{

  bool LedBus_RMT::init(void)
  {
    if (_config.pin_data < 0) {
      return false;
    }
#if M5UNIFIED_RMT_VERSION == 2
    if (_led_encoder && _rmt_ch_handle) {
      return true;
    }

    rmt_tx_channel_config_t rmt_tx;
    memset(&rmt_tx, 0, sizeof(rmt_tx));
    rmt_tx.resolution_hz = _config.frequency;
    rmt_tx.gpio_num = static_cast<gpio_num_t>(_config.pin_data);
    rmt_tx.clk_src = RMT_CLK_SRC_DEFAULT;
    rmt_tx.mem_block_symbols = 64;
    rmt_tx.trans_queue_depth = 4; // set the number of transactions that can be pending in the background
    if (ESP_OK == rmt_new_tx_channel(&rmt_tx, &_rmt_ch_handle)) {
      led_strip_encoder_config_t encoder_config;
      uint32_t freq = _config.frequency / 1000; // convert to kHz.
      encoder_config.t0h_tick = (_config.t0h_ns * freq) / 1000000;
      encoder_config.t0l_tick = (_config.t0l_ns * freq) / 1000000;
      encoder_config.t1h_tick = (_config.t1h_ns * freq) / 1000000;
      encoder_config.t1l_tick = (_config.t1l_ns * freq) / 1000000;
      encoder_config.reset_tick = (_config.reset_us * freq) / 1000;

      if (ESP_OK == rmt_new_led_strip_encoder(&encoder_config, &_led_encoder)) {
        if (ESP_OK == rmt_enable(_rmt_ch_handle)) {
          return true;
        }
        rmt_del_led_strip_encoder(_led_encoder);
        _led_encoder = nullptr;
      }
      rmt_del_channel(_rmt_ch_handle);
      _rmt_ch_handle = nullptr;
    }
#elif M5UNIFIED_RMT_VERSION == 1
#endif
    return false;
  }
  void LedBus_RMT::release(void)
  {
#if M5UNIFIED_RMT_VERSION == 2
    auto rmt_ch_handle = _rmt_ch_handle;
    auto led_encoder = _led_encoder;
    _rmt_ch_handle = nullptr;
    _led_encoder = nullptr;

    if (led_encoder) {
      rmt_del_led_strip_encoder(led_encoder);
    }
    if (rmt_ch_handle) {
      rmt_disable(rmt_ch_handle);
      rmt_del_channel(rmt_ch_handle);
    }
#else
#endif
  }

  void LedBus_RMT::write(const uint8_t* data, size_t length)
  {
#if M5UNIFIED_RMT_VERSION == 2
    if (_rmt_ch_handle && _led_encoder) {
      rmt_tx_wait_all_done(_rmt_ch_handle, portMAX_DELAY);
      rmt_transmit_config_t tx_config;
      memset(&tx_config, 0, sizeof(tx_config));
      // tx_config.loop_count = 0;
      rmt_transmit(_rmt_ch_handle, _led_encoder, data, length, &tx_config);
    }
#else
#endif
  }

//----------------------------------------------------------------------------

  bool LED_Strip_Class::begin(void)
  {
    if (!_bus)
    {
      return false;
    }
    if (!_bus->init())
    {
      return false;
    }
    _rgb_buffer.resize(_config.led_count);
    _send_buffer.resize(_config.led_count * _config.byte_per_led);

    return true;
  }

  void LED_Strip_Class::setColors(const RGBColor* values, size_t index, size_t length)
  {
    if (index + length > _config.led_count)
    {
      length = _config.led_count - index;
    }
    std::copy(values, values + length, _rgb_buffer.begin() + index);
  }
  void LED_Strip_Class::setBrightness(const uint8_t brightness)
  {
    _brightness = brightness;
  }
  void LED_Strip_Class::display(void)
  {
    int g_idx = 0;
    int b_idx = 1;
    int r_idx = 2;
    switch (_config.color_order) {
    case config_t::color_order_rgb:  r_idx = 0; g_idx = 1; b_idx = 2;  break;
    case config_t::color_order_rbg:  r_idx = 0; b_idx = 1; g_idx = 2;  break;
    case config_t::color_order_grb:  g_idx = 0; r_idx = 1; b_idx = 2;  break;
    case config_t::color_order_gbr:  g_idx = 0; b_idx = 1; r_idx = 2;  break;
    case config_t::color_order_brg:  b_idx = 0; r_idx = 1; g_idx = 2;  break;
    case config_t::color_order_bgr:  b_idx = 0; g_idx = 1; r_idx = 2;  break;
    default:  break;
    }

    uint32_t br = _brightness + 1;
    br = br * br;

    auto dst = _send_buffer.data();
    for (size_t i = 0; i < _config.led_count; i++)
    {
      uint8_t r = _rgb_buffer[i].R8();
      uint8_t g = _rgb_buffer[i].G8();
      uint8_t b = _rgb_buffer[i].B8();
      r = (r * br) >> 16;
      g = (g * br) >> 16;
      b = (b * br) >> 16;
      switch (_config.byte_per_led)
      {
      case 3: // RGB
        dst[r_idx] = r;
        dst[g_idx] = g;
        dst[b_idx] = b;
        dst += 3;
        break;

      // TODO: Support formats other than 3 bytes if needed
      default:
        break;
      }
    }
    _bus->write(_send_buffer.data(), _send_buffer.size());
  }
}
