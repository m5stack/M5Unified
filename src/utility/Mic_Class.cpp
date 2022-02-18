// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Mic_Class.hpp"

#include "../M5Unified.hpp"

#if __has_include (<esp_idf_version.h>)
 #include <esp_idf_version.h>
 #if ESP_IDF_VERSION_MAJOR >= 4
  #define NON_BREAK ;[[fallthrough]];
 #endif
#endif

#ifndef NON_BREAK
#define NON_BREAK ;
#endif

#include <sdkconfig.h>
#include <esp_log.h>
#include <math.h>

namespace m5
{
#if defined (ESP_IDF_VERSION_VAL)
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
  #define COMM_FORMAT_I2S (I2S_COMM_FORMAT_STAND_I2S)
  #define COMM_FORMAT_MSB (I2S_COMM_FORMAT_STAND_MSB)
 #endif
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 3)
  #define SAMPLE_RATE_TYPE uint32_t
 #endif
#endif

#ifndef COMM_FORMAT_I2S
#define COMM_FORMAT_I2S (I2S_COMM_FORMAT_I2S)
#define COMM_FORMAT_MSB (I2S_COMM_FORMAT_I2S_MSB)
#endif

#ifndef SAMPLE_RATE_TYPE
#define SAMPLE_RATE_TYPE int
#endif

  uint32_t Mic_Class::_calc_rec_rate(void) const
  {
    int rate = (_cfg.sample_rate * _cfg.over_sampling);
    if (_cfg.use_adc) { rate *= 1.004f; }
    return rate;
  }

  esp_err_t Mic_Class::_setup_i2s(void)
  {
    i2s_driver_uninstall(_cfg.i2s_port);

    if (_cfg.pin_data_in  < 0) { return ESP_FAIL; }

    SAMPLE_RATE_TYPE sample_rate = _calc_rec_rate();
/*
 ESP-IDF ver4系にて I2S_MODE_ADC_BUILT_IN を使用するとサンプリングレートが正しく反映されない不具合があったため、特殊な対策を実装している。
 ・指定するサンプリングレートの値を1/16にする
 ・I2S_MODE_ADC_BUILT_INを使用せずに初期化を行う
 ・最後にI2S0のレジスタを操作してADCモードを有効にする。
*/
    if (_cfg.use_adc) { sample_rate >>= 4; }

    i2s_config_t i2s_config;
    memset(&i2s_config, 0, sizeof(i2s_config_t));
    i2s_config.mode                 = _cfg.use_adc
                                    ? (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX )
                                    : (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX | (0x1 << 6) ); // 0x1<<6 is I2S_MODE_PDM
                                 // : (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM );
    i2s_config.sample_rate          = sample_rate;
    i2s_config.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT;
    i2s_config.communication_format = (i2s_comm_format_t)( COMM_FORMAT_I2S );
    i2s_config.dma_buf_count        = _cfg.dma_buf_count;
    i2s_config.dma_buf_len          = _cfg.dma_buf_len;
    i2s_config.use_apll             = true;

    i2s_pin_config_t pin_config;
    memset(&pin_config, ~0u, sizeof(i2s_pin_config_t)); /// all pin set to I2S_PIN_NO_CHANGE
    pin_config.ws_io_num      = _cfg.pin_ws;
    pin_config.data_in_num    = _cfg.pin_data_in;

    esp_err_t err = i2s_driver_install(_cfg.i2s_port, &i2s_config, 0, nullptr);
    if (err != ESP_OK) { return err; }

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (_cfg.use_adc)
    {
        if (((size_t)_cfg.pin_data_in) > 39) { return ESP_FAIL; }
      static constexpr const uint8_t adc_table[] =
      {
        ADC2_CHANNEL_1 , // GPIO  0
        255            ,
        ADC2_CHANNEL_2 , // GPIO  2
        255            ,
        ADC2_CHANNEL_0 , // GPIO  4
        255, 255, 255, 255, 255, 255, 255,
        ADC2_CHANNEL_5 , // GPIO 12
        ADC2_CHANNEL_4 , // GPIO 13
        ADC2_CHANNEL_6 , // GPIO 14
        ADC2_CHANNEL_3 , // GPIO 15
        255, 255, 255, 255, 255, 255, 255, 255, 255,
        ADC2_CHANNEL_8 , // GPIO 25
        ADC2_CHANNEL_9 , // GPIO 26
        ADC2_CHANNEL_7 , // GPIO 27
        255, 255, 255, 255,
        ADC1_CHANNEL_4 , // GPIO 32
        ADC1_CHANNEL_5 , // GPIO 33
        ADC1_CHANNEL_6 , // GPIO 34
        ADC1_CHANNEL_7 , // GPIO 35
        ADC1_CHANNEL_0 , // GPIO 36
        ADC1_CHANNEL_1 , // GPIO 37
        ADC1_CHANNEL_2 , // GPIO 38
        ADC1_CHANNEL_3 , // GPIO 39
      };
      int adc_ch = adc_table[_cfg.pin_data_in];
      if (adc_ch == 255) { return ESP_FAIL; }

      adc_unit_t unit = _cfg.pin_data_in >= 32 ? ADC_UNIT_1 : ADC_UNIT_2;
      adc_set_data_width(unit, ADC_WIDTH_12Bit);
      err = i2s_set_adc_mode(unit, (adc1_channel_t)adc_ch);
      if (unit == ADC_UNIT_1)
      {
        adc1_config_channel_atten((adc1_channel_t)adc_ch, ADC_ATTEN_11db);
      }
      else
      {
        adc2_config_channel_atten((adc2_channel_t)adc_ch, ADC_ATTEN_11db);
      }
      if (_cfg.i2s_port == I2S_NUM_0)
      { /// レジスタを操作してADCモードの設定を有効にする ;
        I2S0.conf2.lcd_en = true;
        I2S0.conf.rx_right_first = 0;
        I2S0.conf.rx_msb_shift = 0;
        I2S0.conf.rx_mono = 0;
        I2S0.conf.rx_short_sync = 0;
        I2S0.fifo_conf.rx_fifo_mod = true;
        I2S0.conf_chan.rx_chan_mod = true;
      }
    }
    else
#endif
    {
      err = i2s_set_pin(_cfg.i2s_port, &pin_config);
    }

    return err;
  }

  void Mic_Class::input_task(void* args)
  {
    auto self = (Mic_Class*)args;

    int oversampling = self->_cfg.over_sampling;
    if (     oversampling < 1) { oversampling = 1; }
    else if (oversampling > 8) { oversampling = 8; }
    int32_t gain = self->_cfg.magnification;
    const float f_gain = (float)gain / oversampling;
    int32_t offset = self->_cfg.input_offset;
    size_t src_idx = ~0u;
    size_t src_len = 0;
    int32_t value = 0;
    int32_t prev_value = 0;
    int32_t os_remain = oversampling;
    const size_t dma_buf_len = self->_cfg.dma_buf_len;
    int16_t* src_buf = (int16_t*)alloca(dma_buf_len * sizeof(int16_t));

    while (self->_task_running)
    {
      self->_rec_info[0] = self->_rec_info[1];
      self->_rec_info[1].length = 0;
      size_t dst_remain = self->_rec_info[0].length;
      if (!dst_remain)
      {
        self->_is_recording = false;
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        i2s_read(self->_cfg.i2s_port, src_buf, dma_buf_len, &src_len, 100 / portTICK_RATE_MS);
        src_idx = 0;
        src_len = 0;
        value = 0;
        continue;
      }

      for (;;)
      {
        if (src_idx >= src_len)
        {
          i2s_read(self->_cfg.i2s_port, src_buf, dma_buf_len, &src_len, 100 / portTICK_RATE_MS);
          src_len >>= 1;
          src_idx = 0;
        }

        if (self->_cfg.use_adc)
        {
          do
          {
            value += (src_buf[src_idx^1] & 0x0FFF) + (offset - 2048);
            ++src_idx;
          } while (--os_remain && (src_idx < src_len));
        }
        else
        {
          do
          {
            value += src_buf[src_idx] + offset;
            ++src_idx;
          } while (--os_remain && (src_idx < src_len));
        }
        if (os_remain) continue;
        os_remain = oversampling;

        int32_t noise_filter = self->_cfg.noise_filter_level;
        if (noise_filter)
        {
          value = (value + prev_value * noise_filter) / (noise_filter + 1);
          prev_value = value;
        }

        value = value * f_gain;

        if (     value < INT16_MIN) { value = INT16_MIN; }
        else if (value > INT16_MAX) { value = INT16_MAX; }

        auto &rec_info = self->_rec_info[0];
        if (rec_info.is_16bit)
        {
          auto dst = (int16_t*)(rec_info.data);
          *dst++ = value;
          rec_info.data = dst;
        }
        else
        {
          auto dst = (uint8_t*)(rec_info.data);
          *dst++ = (value >> 8) + 128;
          rec_info.data = dst;
        }
        value = 0;
        if (--dst_remain == 0) break;
      }
    }
    i2s_stop(self->_cfg.i2s_port);

    self->_task_handle = nullptr;
    vTaskDelete(nullptr);
  }

  bool Mic_Class::begin(void)
  {
    if (_task_running)
    {
      if (_rec_sample_rate == _cfg.sample_rate)
      {
        return true;
      }
      while (isRecording()) { vTaskDelay(1); }
      end();
      _rec_sample_rate = _cfg.sample_rate;
    }

    bool res = true;
    if (_cb_set_enabled) { res = _cb_set_enabled(_cb_set_enabled_args, true); }

    res = (ESP_OK == _setup_i2s()) && res;
    if (res)
    {
      size_t stack_size = 1024+(_cfg.dma_buf_len * sizeof(int16_t));
      _task_running = true;
      if (_cfg.task_pinned_core >= 0 && _cfg.task_pinned_core < portNUM_PROCESSORS)
      {
        xTaskCreatePinnedToCore(input_task, "mic_task", stack_size, this, _cfg.task_priority, &_task_handle, _cfg.task_pinned_core);
      }
      else
      {
        xTaskCreate(input_task, "mic_task", stack_size, this, _cfg.task_priority, &_task_handle);
      }
    }

    return res;
  }

  void Mic_Class::end(void)
  {
    if (!_task_running) { return; }
    _task_running = false;
    if (_task_handle)
    {
      if (_task_handle) { xTaskNotifyGive(_task_handle); }
      do { vTaskDelay(1); } while (_task_handle);
    }

    if (_cb_set_enabled) { _cb_set_enabled(_cb_set_enabled_args, false); }
  }

  bool Mic_Class::_rec_raw(void* recdata, size_t array_len, bool flg_16bit, uint32_t sample_rate)
  {
    if (array_len == 0) { return true; }
    recording_info_t info;
    info.data = recdata;
    info.length = array_len;
    info.is_16bit = flg_16bit;

    _cfg.sample_rate = sample_rate;

    if (!begin()) { return false; }
    while (_rec_info[1].length) { taskYIELD(); }
    _rec_info[1] = info;
    if (this->_task_handle)
    {
      _is_recording = true;
      xTaskNotifyGive(this->_task_handle);
    }
    return true;
  }
}
