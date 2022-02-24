// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Speaker_Class.hpp"

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

  const uint8_t Speaker_Class::_default_tone_wav[2] = { 192, 64 }; // square wave data

  esp_err_t Speaker_Class::_setup_i2s(void)
  {
    i2s_driver_uninstall(_cfg.i2s_port);

    if (_cfg.pin_data_out < 0) { return ESP_FAIL; }

    SAMPLE_RATE_TYPE sample_rate = _cfg.sample_rate;

/*
 ESP-IDF ver4系にて I2S_MODE_DAC_BUILT_IN を使用するとサンプリングレートが正しく反映されない不具合があったため、特殊な対策を実装している。
 ・指定するサンプリングレートの値を1/16にする
 ・I2S_MODE_DAC_BUILT_INを使用せずに初期化を行う
 ・最後にI2S0のレジスタを操作してDACモードを有効にする。
*/
    if (_cfg.use_dac) { sample_rate >>= 4; }

    i2s_config_t i2s_config;
    memset(&i2s_config, 0, sizeof(i2s_config_t));
    i2s_config.mode                 = (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_TX );
    i2s_config.sample_rate          = sample_rate;
    i2s_config.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format       = _cfg.stereo
                                    ? I2S_CHANNEL_FMT_RIGHT_LEFT
                                    : I2S_CHANNEL_FMT_ONLY_RIGHT;
    i2s_config.communication_format = (i2s_comm_format_t)( COMM_FORMAT_I2S );
    i2s_config.dma_buf_count        = _cfg.dma_buf_count;
    i2s_config.dma_buf_len          = _cfg.dma_buf_len;
    i2s_config.tx_desc_auto_clear   = true;

    i2s_pin_config_t pin_config;
    memset(&pin_config, ~0u, sizeof(i2s_pin_config_t)); /// all pin set to I2S_PIN_NO_CHANGE
    pin_config.bck_io_num     = _cfg.pin_bck;
    pin_config.ws_io_num      = _cfg.pin_ws;
    pin_config.data_out_num   = _cfg.pin_data_out;

    esp_err_t err = i2s_driver_install(_cfg.i2s_port, &i2s_config, 0, nullptr);
    if (err != ESP_OK) { return err; }

    i2s_zero_dma_buffer(_cfg.i2s_port);

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (_cfg.use_dac)
    {
      i2s_dac_mode_t dac_mode = i2s_dac_mode_t::I2S_DAC_CHANNEL_BOTH_EN;
      if (!_cfg.stereo)
      {
        dac_mode = (_cfg.pin_data_out == GPIO_NUM_25)
                ? i2s_dac_mode_t::I2S_DAC_CHANNEL_RIGHT_EN // for GPIO 25
                : i2s_dac_mode_t::I2S_DAC_CHANNEL_LEFT_EN; // for GPIO 26
      }
      err = i2s_set_dac_mode(dac_mode);
      if (_cfg.i2s_port == I2S_NUM_0)
      { /// レジスタを操作してDACモードの設定を有効にする ;
        I2S0.conf2.lcd_en = true;
        I2S0.conf.tx_right_first = true;
        I2S0.conf.tx_msb_shift = 0;
        I2S0.conf.tx_short_sync = 0;
      }
    }
    else
#endif
    {
      err = i2s_set_pin(_cfg.i2s_port, &pin_config);
    }

    return err;
  }

  void Speaker_Class::output_task(void* args)
  {
    auto self = (Speaker_Class*)args;

    const bool out_stereo = self->_cfg.stereo;
    const int32_t spk_sample_rate = self->_cfg.sample_rate;
    const size_t dma_buf_len = self->_cfg.dma_buf_len & ~1;
    int nodata_count = 0;
    int32_t dac_offset = self->_cfg.dac_zero_level << 8;
    int32_t surplus[2] = { 0, 0 };
    bool flg_i2s_started = false;

    union
    {
      float* float_buf;
      int16_t* sound_buf;
    };
    float_buf = (float*)alloca(dma_buf_len * sizeof(float));

    while (self->_task_running)
    {
      if (++nodata_count > self->_cfg.dma_buf_count * 2)
      {
        nodata_count = 0;
        flg_i2s_started = false;
        i2s_stop(self->_cfg.i2s_port);
        // i2s_zero_dma_buffer(self->_cfg.i2s_port);
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
        if (self->_cfg.use_dac)
        {
          i2s_set_dac_mode(i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE);
        }
#endif
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        continue;
      }

      memset(float_buf, 0, dma_buf_len * sizeof(float));

      for (size_t ch = 0; ch < sound_channel_max; ++ch)
      {
        if (0 == (self->_play_channel_bits.load() & (1 << ch))) { continue; }
        nodata_count = 0;

        size_t idx = 0;
        auto ch_info = &(self->_ch_info[ch]);
        wav_info_t* current_wav = &(ch_info->wavinfo[!ch_info->flip]);
        wav_info_t* next_wav = &(ch_info->wavinfo[ch_info->flip]);

        bool in_stereo = current_wav->is_stereo;
        int32_t tmp = ch_info->volume;
        tmp *= tmp;
        if (!current_wav->is_16bit) { tmp <<= 8; }
        if (out_stereo) { tmp <<= 1; }
        float ch_v = tmp;
        const void* data = current_wav->data;
        auto in_rate = current_wav->sample_rate;
        do
        {
          if (current_wav->repeat == 0 || next_wav->stop_current)
          {
            bool clear_idx = (next_wav->repeat == 0
                          || !next_wav->no_clear_index
                          || (next_wav->data != current_wav->data));
            current_wav->clear();
            ch_info->flip = !ch_info->flip;
            xSemaphoreGive(self->_task_semaphore);
            std::swap(current_wav, next_wav);

            if (clear_idx)
            {
              ch_info->index = 0;
              ch_info->diff  = 0;
              if (current_wav->repeat == 0)
              {
                self->_play_channel_bits.fetch_and(~(1 << ch));
                if (current_wav->repeat == 0) { break; }
                self->_play_channel_bits.fetch_or(1 << ch);
              }
            }
            in_stereo = current_wav->is_stereo;
            int32_t tmp = ch_info->volume;
            tmp *= tmp;
            if (!current_wav->is_16bit) { tmp <<= 8; }
            if (out_stereo) { tmp <<= 1; }
            ch_v = tmp;
            data = current_wav->data;
            in_rate = current_wav->sample_rate;
          }

          int32_t l, r;
          size_t ch_index = ch_info->index;
          if (current_wav->is_16bit)
          {
            auto wav = (const int16_t*)data;
            l = wav[ch_index];
            r = wav[ch_index + in_stereo];
            if (!current_wav->is_signed)
            {
              l = (l & 0xFFFF) + INT16_MIN;
              r = (r & 0xFFFF) + INT16_MIN;
            }
          }
          else
          {
            auto wav = (const int8_t*)data;
            l = wav[ch_index];
            r = wav[ch_index + in_stereo];
            if (!current_wav->is_signed)
            {
              l = (l & 0xFF) + INT8_MIN;
              r = (r & 0xFF) + INT8_MIN;
            }
          }

          size_t ch_diff = ch_info->diff;
          if (out_stereo)
          {
            float fl = l * ch_v;
            float fr = r * ch_v;
            do
            {
              float_buf[  idx] += fl;
              float_buf[++idx] += fr;
              ch_diff += in_rate;
            } while (++idx < dma_buf_len && ch_diff < spk_sample_rate);
          }
          else
          {
            float fl = l + r;
            fl *= ch_v;
            do
            {
              float_buf[idx] += fl;
              ch_diff += in_rate;
            } while (++idx < dma_buf_len && ch_diff < spk_sample_rate);
          }

          if (ch_diff >= spk_sample_rate)
          {
            size_t tmp = ch_diff / spk_sample_rate;
            ch_diff -= tmp * spk_sample_rate;
            ch_index += tmp * (in_stereo + 1);

            while (ch_index >= current_wav->length)
            {
              ch_index -= current_wav->length;
              if (current_wav->repeat != ~0u)
              {
                if (0 == --current_wav->repeat) { break; }
              }
            }
          }
          ch_info->index = ch_index;
          ch_info->diff = ch_diff;
        } while (idx < dma_buf_len);
      }

      float volume = ((float)(self->_master_volume * self->_master_volume * self->_cfg.magnification)) / 0x2000000u;
      size_t idx = 0;
      if (self->_cfg.use_dac)
      {
        volume /= 256;
        const bool zero_bias = (self->_cfg.dac_zero_level == 0);
        if (zero_bias && nodata_count)
        {
          do
          {
            dac_offset = (dac_offset * 255) >> 8;
            int32_t v = dac_offset + surplus[idx & out_stereo];
            surplus[idx & out_stereo] = v & 255;
            sound_buf[idx ^ 1] = v;
          } while (++idx < dma_buf_len);
        }
        else
        {
          do
          {
            int32_t v = volume * float_buf[idx];
            if (zero_bias)
            {
              int32_t vabs = abs(v);
              if (vabs > INT16_MAX) { vabs = INT16_MAX; }
              if (--dac_offset < vabs) { dac_offset = vabs; }
            }
            v += surplus[idx & out_stereo] + dac_offset;
            surplus[idx & out_stereo] = v & 255;
            if (v < 0) { v = 0; }
            else if (v > UINT16_MAX) { v = UINT16_MAX; }
            sound_buf[idx ^ 1] = v;
          } while (++idx < dma_buf_len);
        }
      }
      else if (self->_cfg.buzzer)
      {
        volume /= 256;
        int32_t tmp = surplus[0];
        do
        {
          int32_t v = volume * float_buf[idx];
          uint_fast16_t bitdata = 0;
          uint_fast16_t bit = 0x8000;
          v += 0x8000;
          do
          {
            if ((tmp += v) >= 0)
            {
              tmp -= 0x10000;
              bitdata |= bit;
            }
          } while (bit >>= 1);
          sound_buf[idx] = bitdata;
        } while (++idx < dma_buf_len);
        surplus[0] = tmp;
      }
      else
      {
        do
        {
          int32_t v = volume * float_buf[idx];
          v += surplus[idx & out_stereo];
          surplus[idx & out_stereo] = v & 255;
          v >>= 8;
          if (v < INT16_MIN) { v = INT16_MIN; }
          else if (v > INT16_MAX) { v = INT16_MAX; }
          sound_buf[idx] = v;
        } while (++idx < dma_buf_len);
      }

      if (!flg_i2s_started)
      {
        flg_i2s_started = true;
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
        if (self->_cfg.use_dac)
        {
          i2s_dac_mode_t dac_mode = i2s_dac_mode_t::I2S_DAC_CHANNEL_BOTH_EN;
          if (!self->_cfg.stereo)
          {
            dac_mode = (self->_cfg.pin_data_out == GPIO_NUM_25)
                    ? i2s_dac_mode_t::I2S_DAC_CHANNEL_RIGHT_EN // for GPIO 25
                    : i2s_dac_mode_t::I2S_DAC_CHANNEL_LEFT_EN; // for GPIO 26
          }
          i2s_set_dac_mode(dac_mode);
        }
#endif
        i2s_start(self->_cfg.i2s_port);
      }

      size_t write_bytes;
      i2s_write(self->_cfg.i2s_port, sound_buf, dma_buf_len * sizeof(int16_t), &write_bytes, portMAX_DELAY);
    }
    i2s_stop(self->_cfg.i2s_port);
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (self->_cfg.use_dac)
    {
      i2s_set_dac_mode(i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE);
      m5gfx::gpio_lo(self->_cfg.pin_data_out);
      m5gfx::pinMode(self->_cfg.pin_data_out, m5gfx::pin_mode_t::output);
    }
#endif
    self->_task_handle = nullptr;
    vTaskDelete(nullptr);
  }

  bool Speaker_Class::begin(void)
  {
    if (_task_running) { return true; }

    if (_task_semaphore == nullptr) { _task_semaphore = xSemaphoreCreateBinary(); }

    bool res = true;
    if (_cb_set_enabled) { res = _cb_set_enabled(_cb_set_enabled_args, true); }

    res = (ESP_OK == _setup_i2s()) && res;
    if (res)
    {
      size_t stack_size = 1024 + (_cfg.dma_buf_len * sizeof(float));
      _task_running = true;
      if (_cfg.task_pinned_core >= 0 && _cfg.task_pinned_core < portNUM_PROCESSORS)
      {
        xTaskCreatePinnedToCore(output_task, "spk_task", stack_size, this, _cfg.task_priority, &_task_handle, _cfg.task_pinned_core);
      }
      else
      {
        xTaskCreate(output_task, "spk_task", stack_size, this, _cfg.task_priority, &_task_handle);
      }
    }

    return res;
  }

  void Speaker_Class::end(void)
  {
    if (!_task_running) { return; }
    _task_running = false;
    if (_task_handle)
    {
      if (_task_handle) { xTaskNotifyGive(_task_handle); }
      do { vTaskDelay(1); } while (_task_handle);
    }
    stop();
    if (_cb_set_enabled) { _cb_set_enabled(_cb_set_enabled_args, false); }
  }

  void Speaker_Class::stop(void)
  {
    wav_info_t tmp;
    tmp.stop_current = 1;
    for (size_t ch = 0; ch < sound_channel_max; ++ch)
    {
      auto chinfo = &_ch_info[ch];
      chinfo->wavinfo[chinfo->flip] = tmp;
    }
  }

  void Speaker_Class::stop(uint8_t ch)
  {
    if ((size_t)ch >= sound_channel_max)
    {
      stop();
    }
    else
    {
      wav_info_t tmp;
      tmp.stop_current = 1;
      auto chinfo = &_ch_info[ch];
      chinfo->wavinfo[chinfo->flip] = tmp;
    }
  }

  void Speaker_Class::wav_info_t::clear(void)
  {
    length = 0;
    data = nullptr;
    sample_rate = 0;
    flg = 0;
    repeat = 0;
  }

  bool Speaker_Class::_set_next_wav(size_t ch, const wav_info_t& wav)
  {
    auto chinfo = &_ch_info[ch];
    uint8_t chmask = 1 << ch;
    if (!wav.stop_current)
    {
      if (isPlaying(ch) > 1) { return false; }
      while ((_play_channel_bits.load() & chmask) && (chinfo->wavinfo[chinfo->flip].repeat)) { xSemaphoreTake(_task_semaphore, 1); }
    }
    chinfo->wavinfo[chinfo->flip] = wav;
    _play_channel_bits.fetch_or(chmask);

    xTaskNotifyGive(_task_handle);
    return true;
  }

  bool Speaker_Class::_play_raw(const void* wav_data, size_t array_len, bool flg_16bit, bool flg_signed, uint32_t sample_rate, bool flg_stereo, uint32_t repeat_count, int channel, bool stop_current_sound, bool no_clear_index)
  {
    if (!begin() || (_task_handle == nullptr)) { return true; }
    if (array_len == 0) { return true; }
    size_t ch = (size_t)channel;
    if (ch >= sound_channel_max)
    {
      size_t bits = _play_channel_bits.load();
      for (ch = sound_channel_max - 1; ch < sound_channel_max; --ch)
      {
        if (0 == ((bits >> ch) & 1)) { break; }
      }
      if (ch >= sound_channel_max) { return true; }
    }
    wav_info_t info;
    info.data = wav_data;
    info.length = array_len;
    info.repeat = repeat_count ? repeat_count : ~0u;
    info.sample_rate = sample_rate;
    info.is_stereo = flg_stereo;
    info.is_16bit = flg_16bit;
    info.is_signed = flg_signed;
    info.stop_current = stop_current_sound;
    info.no_clear_index = no_clear_index;

    return _set_next_wav(ch, info);
  }
}
