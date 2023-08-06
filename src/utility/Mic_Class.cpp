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

#if __has_include(<sdkconfig.h>)
#include <sdkconfig.h>
#include <esp_log.h>
#include <math.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace m5
{
#if defined ( ESP_PLATFORM )
#if defined (ESP_IDF_VERSION_VAL)
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
  #define COMM_FORMAT_I2S (I2S_COMM_FORMAT_STAND_I2S)
  #define COMM_FORMAT_MSB (I2S_COMM_FORMAT_STAND_MSB)
 #endif
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 3)
  #define SAMPLE_RATE_TYPE uint32_t
 #endif
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #define MIC_CLASS_ADC_WIDTH_BITS ADC_WIDTH_BIT_12
  #define MIC_CLASS_ADC_ATTEN_DB ADC_ATTEN_DB_11
 #endif
#endif

#ifndef COMM_FORMAT_I2S
 #define COMM_FORMAT_I2S (I2S_COMM_FORMAT_I2S)
 #define COMM_FORMAT_MSB (I2S_COMM_FORMAT_I2S_MSB)
#endif

#ifndef SAMPLE_RATE_TYPE
 #define SAMPLE_RATE_TYPE int
#endif

#ifndef MIC_CLASS_ADC_WIDTH_BITS
 #define MIC_CLASS_ADC_WIDTH_BITS ADC_WIDTH_12Bit
 #define MIC_CLASS_ADC_ATTEN_DB ADC_ATTEN_11db
#endif


  uint32_t Mic_Class::_calc_rec_rate(void) const
  {
    int rate = (_cfg.sample_rate * _cfg.over_sampling);
    return rate;
  }

  esp_err_t Mic_Class::_setup_i2s(void)
  {
    if (_cfg.pin_data_in  < 0) { return ESP_FAIL; }

    SAMPLE_RATE_TYPE sample_rate = _calc_rec_rate();
/*
 ESP-IDF ver4系にて I2S_MODE_ADC_BUILT_IN を使用するとサンプリングレートが正しく反映されない不具合があったため、特殊な対策を実装している。
 ・指定するサンプリングレートの値を1/16にする
 ・I2S_MODE_ADC_BUILT_INを使用せずに初期化を行う
 ・最後にI2S0のレジスタを操作してADCモードを有効にする。
*/
    bool use_pdm = (_cfg.pin_bck < 0);
    if (_cfg.use_adc) { sample_rate >>= 4; use_pdm = false;}
//  ESP_LOGV("Mic","sampling rate:%" PRIu32 , sample_rate);
    i2s_config_t i2s_config;
    memset(&i2s_config, 0, sizeof(i2s_config_t));
    i2s_config.mode                 = use_pdm
                                //  ? (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM );
                                    ? (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX | (0x1 << 6) ) // 0x1<<6 is I2S_MODE_PDM
                                    : (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX );
    i2s_config.sample_rate          = sample_rate;
    i2s_config.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format       = _cfg.stereo ? I2S_CHANNEL_FMT_RIGHT_LEFT : I2S_CHANNEL_FMT_ONLY_RIGHT;
    i2s_config.communication_format = (i2s_comm_format_t)( COMM_FORMAT_I2S );
    i2s_config.dma_buf_count        = _cfg.dma_buf_count;
    i2s_config.dma_buf_len          = _cfg.dma_buf_len;

    i2s_pin_config_t pin_config;
    memset(&pin_config, ~0u, sizeof(i2s_pin_config_t)); /// all pin set to I2S_PIN_NO_CHANGE
#if defined (ESP_IDF_VERSION_VAL)
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 1)
    pin_config.mck_io_num     = _cfg.pin_mck;
 #endif
#endif
    pin_config.bck_io_num     = _cfg.pin_bck;
    pin_config.ws_io_num      = _cfg.pin_ws;
    pin_config.data_in_num    = _cfg.pin_data_in;

    esp_err_t err;
    if (ESP_OK != (err = i2s_driver_install(_cfg.i2s_port, &i2s_config, 0, nullptr)))
    {
      i2s_driver_uninstall(_cfg.i2s_port);
      err = i2s_driver_install(_cfg.i2s_port, &i2s_config, 0, nullptr);
    }
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
      adc_set_data_width(unit, MIC_CLASS_ADC_WIDTH_BITS);
      err = i2s_set_adc_mode(unit, (adc1_channel_t)adc_ch);
      if (unit == ADC_UNIT_1)
      {
        adc1_config_channel_atten((adc1_channel_t)adc_ch, MIC_CLASS_ADC_ATTEN_DB);
      }
      else
      {
        adc2_config_channel_atten((adc2_channel_t)adc_ch, MIC_CLASS_ADC_ATTEN_DB);
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

  void Mic_Class::mic_task(void* args)
  {
    auto self = (Mic_Class*)args;
    i2s_start(self->_cfg.i2s_port);

    int oversampling = self->_cfg.over_sampling;
    if (     oversampling < 1) { oversampling = 1; }
    else if (oversampling > 8) { oversampling = 8; }
    oversampling <<= 1;
    int32_t gain = self->_cfg.magnification;
    const float f_gain = (float)gain / oversampling;
    int32_t offset = 0;
    size_t src_idx = ~0u;
    size_t src_len = 0;
    int32_t sum_value[4] = { 0,0 };
    int32_t prev_value[2] = { 0, 0 };
    const bool in_stereo = self->_cfg.stereo;
    int32_t os_remain = oversampling;
    const size_t dma_buf_len = self->_cfg.dma_buf_len;
    int16_t* src_buf = (int16_t*)alloca(dma_buf_len * sizeof(int16_t));

    i2s_read(self->_cfg.i2s_port, src_buf, dma_buf_len, &src_len, portTICK_PERIOD_MS);
    i2s_read(self->_cfg.i2s_port, src_buf, dma_buf_len, &src_len, portTICK_PERIOD_MS);

    while (self->_task_running)
    {
      bool rec_flip = self->_rec_flip;
      recording_info_t* current_rec = &(self->_rec_info[!rec_flip]);
      recording_info_t* next_rec    = &(self->_rec_info[ rec_flip]);

      size_t dst_remain = current_rec->length;
      if (dst_remain == 0)
      {
        rec_flip = !rec_flip;
        self->_rec_flip = rec_flip;
        xSemaphoreGive(self->_task_semaphore);
        std::swap(current_rec, next_rec);
        dst_remain = current_rec->length;
        if (dst_remain == 0)
        {
          self->_is_recording = false;
          ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
          src_idx = ~0u;
          src_len = 0;
          sum_value[0] = 0;
          sum_value[1] = 0;
          continue;
        }
      }
      self->_is_recording = true;

      for (;;)
      {
        if (src_idx >= src_len)
        {
          i2s_read(self->_cfg.i2s_port, src_buf, dma_buf_len, &src_len, 100 / portTICK_PERIOD_MS);
          src_len >>= 1;
          src_idx = 0;
        }

        if (self->_cfg.use_adc)
        {
          do
          {
#if defined (CONFIG_IDF_TARGET_ESP32)
            sum_value[1 - (src_idx & 1)] += (src_buf[src_idx] & 0x0FFF) - 2048;
#else
            sum_value[src_idx & 1] += (src_buf[src_idx] & 0x0FFF) - 2048;
#endif
            ++src_idx;
          } while (--os_remain && (src_idx < src_len));
        }
        else
        {
          do
          {
#if defined (CONFIG_IDF_TARGET_ESP32)
            sum_value[1 - (src_idx & 1)] += src_buf[src_idx];
#else
            sum_value[src_idx & 1] += src_buf[src_idx];
#endif
            ++src_idx;
          } while (--os_remain && (src_idx < src_len));
        }
        if (os_remain) { continue; }
        os_remain = oversampling;

        auto sv0 = sum_value[0];
        auto sv1 = sum_value[1];
        auto value_tmp = sv0 + sv1;
        sum_value[0] += offset;
        sum_value[1] += offset;
        // Automatic zero level adjustment
        offset = (32 + offset * 62 - value_tmp) >> 6;

        int32_t noise_filter = self->_cfg.noise_filter_level;
        if (noise_filter)
        {
          for (int i = 0; i < 2; ++i)
          {
            int32_t v = (sum_value[i] * (256 - noise_filter) + prev_value[i] * noise_filter + 128) >> 8;
            prev_value[i] = v;
            sum_value[i] = v * f_gain;
          }
        }
        else
        {
          for (int i = 0; i < 2; ++i)
          {
            sum_value[i] *= f_gain;
          }
        }

        int output_num = 2;

        if (in_stereo != current_rec->is_stereo)
        {
          if (in_stereo)
          { // stereo -> mono  convert.
            sum_value[0] = (sum_value[0] + sum_value[1] + 1) >> 1;
            output_num = 1;
          }
          else
          { // mono -> stereo  convert.
            auto tmp = sum_value[1];
            sum_value[3] = tmp;
            sum_value[2] = tmp;
            sum_value[1] = sum_value[0];
            output_num = 4;
          }
        }
        for (int i = 0; i < output_num; ++i)
        {
          auto value = sum_value[i];
          if (current_rec->is_16bit)
          {
            if (     value < INT16_MIN+16) { value = INT16_MIN+16; }
            else if (value > INT16_MAX-16) { value = INT16_MAX-16; }
            auto dst = (int16_t*)(current_rec->data);
            *dst++ = value;
            current_rec->data = dst;
          }
          else
          {
            value = ((value + 128) >> 8) + 128;
            if (     value < 0) { value = 0; }
            else if (value > 255) { value = 255; }
            auto dst = (uint8_t*)(current_rec->data);
            *dst++ = value;
            current_rec->data = dst;
          }
        }
        sum_value[0] = 0;
        sum_value[1] = 0;
        dst_remain -= output_num;
        if ((int32_t)dst_remain <= 0)
        {
          current_rec->length = 0;
          break;
        }
      }
    }
    self->_is_recording = false;
    i2s_stop(self->_cfg.i2s_port);

    self->_task_handle = nullptr;
    vTaskDelete(nullptr);
  }

  bool Mic_Class::begin(void)
  {
    if (_task_running)
    {
      auto rate = _calc_rec_rate();
      if (_rec_sample_rate == rate)
      {
        return true;
      }
      do { vTaskDelay(1); } while (isRecording());
      end();
      _rec_sample_rate = rate;
    }

    if (_task_semaphore == nullptr) { _task_semaphore = xSemaphoreCreateBinary(); }

    bool res = true;
    if (_cb_set_enabled) { res = _cb_set_enabled(_cb_set_enabled_args, true); }

    res = (ESP_OK == _setup_i2s()) && res;
    if (res)
    {
      size_t stack_size = 1024 + (_cfg.dma_buf_len * sizeof(int16_t));
      _task_running = true;
#if portNUM_PROCESSORS > 1
      if (_cfg.task_pinned_core < portNUM_PROCESSORS)
      {
        xTaskCreatePinnedToCore(mic_task, "mic_task", stack_size, this, _cfg.task_priority, &_task_handle, _cfg.task_pinned_core);
      }
      else
#endif
      {
        xTaskCreate(mic_task, "mic_task", stack_size, this, _cfg.task_priority, &_task_handle);
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
    i2s_driver_uninstall(_cfg.i2s_port);
  }

  bool Mic_Class::_rec_raw(void* recdata, size_t array_len, bool flg_16bit, uint32_t sample_rate, bool flg_stereo)
  {
    recording_info_t info;
    info.data = recdata;
    info.length = array_len;
    info.is_16bit = flg_16bit;
    info.is_stereo = flg_stereo;

    _cfg.sample_rate = sample_rate;

    if (!begin()) { return false; }
    if (array_len == 0) { return true; }
    while (_rec_info[_rec_flip].length) { xSemaphoreTake(_task_semaphore, 1); }
    _rec_info[_rec_flip] = info;
    if (this->_task_handle)
    {
      xTaskNotifyGive(this->_task_handle);
    }
    return true;
  }
#endif
}
#endif