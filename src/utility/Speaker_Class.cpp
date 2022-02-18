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

  const uint8_t Speaker_Class::_default_tone_wav[2] = { 191, 64 };

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
    i2s_config.use_apll             = true;
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

    const size_t lr_loop = 1 + self->_cfg.stereo;
    const int32_t spk_sample_rate = self->_cfg.sample_rate;
    int nodata_count = 0;
    int32_t dac_offset = self->_cfg.dac_zero_level << 8;
    int32_t surplus[2] = { 0, 0 };
    bool flg_i2s_started = false;
    const size_t dma_buf_len = self->_cfg.dma_buf_len;
    int16_t* sound_buf = (int16_t*)alloca(dma_buf_len * sizeof(int16_t));
    while (self->_task_running)
    {
      size_t sound_buf_index = 0;
      do
      {
        int32_t value[2] = { 0, 0 };
        if (self->_play_channel_bits)
        {
          nodata_count = 0;
          for (size_t idx = 0; idx < sound_channel_max; ++idx)
          {
            if (0 == (self->_play_channel_bits & (1 << idx))) { continue; }
            auto ch_info = &(self->_ch_info[idx]);

            if (ch_info->current_wav.repeat == 0 || ch_info->next_wav.stop_current)
            {
              if (ch_info->next_wav.repeat == 0
              || !ch_info->next_wav.no_clear_index
              || (ch_info->next_wav.data != ch_info->current_wav.data))
              {
                ch_info->index = 0;
                ch_info->diff  = 0;
                if (ch_info->next_wav.repeat == 0)
                {
                  self->_play_channel_bits &= ~(1 << idx);
                  continue;
                }
              }
              ch_info->current_wav = ch_info->next_wav;
              ch_info->next_wav.clear();
            }

            auto current_wav = &(ch_info->current_wav);
            bool stereo = current_wav->is_stereo;
            int32_t l, r;
            int32_t ch_v = ch_info->volume;
            ch_v *= ch_v;
            size_t ch_index = ch_info->index;
            const void* data = current_wav->data;
            if (current_wav->is_16bit)
            {
              auto wav = (const uint16_t*)data;
              l = wav[ch_index         ];
              r = wav[ch_index + stereo];
              if (current_wav->is_signed)
              {
                l = (int16_t)l;
                r = (int16_t)r;
              }
              else
              {
                l += INT16_MIN;
                r += INT16_MIN;
              }
            }
            else
            {
              ch_v <<= 8;
              auto wav = (const uint8_t*)data;
              l = wav[ch_index         ];
              r = wav[ch_index + stereo];
              if (current_wav->is_signed)
              {
                l = (int8_t)l;
                r = (int8_t)r;
              }
              else
              {
                l += INT8_MIN;
                r += INT8_MIN;
              }
            }
            value[0] += l * ch_v;
            value[1] += r * ch_v;

            size_t ch_diff = ch_info->diff + current_wav->sample_rate;
            if (ch_diff >= spk_sample_rate)
            {
              size_t tmp = ch_diff / spk_sample_rate;
              ch_diff -= tmp * spk_sample_rate;
              ch_index += tmp * (stereo + 1);

              while (ch_index >= current_wav->length)
              {
                ch_index -= current_wav->length;
                if (ch_info->current_wav.repeat != ~0u)
                {
                  if (0 == --ch_info->current_wav.repeat) { break; }
                }
              }
            }
            ch_info->index = ch_index;
            ch_info->diff = ch_diff;
          }
          float volume = ((float)(self->_master_volume * self->_master_volume * self->_cfg.magnification)) / (0x1000000u >> self->_cfg.stereo);
          if (self->_cfg.stereo)
          {
            value[0] = volume * value[0];
            value[1] = volume * value[1];
          }
          else
          {
            value[0] = volume * (value[0] + value[1]);
          }
        }

        size_t lr = 0;
        do
        {
          int32_t v = value[lr];
          if (self->_cfg.use_dac)
          {
            v >>= 8;
            if (self->_cfg.dac_zero_level == 0)
            {
              if (!self->_play_channel_bits)
              {
                dac_offset = (dac_offset * 255) >> 8;
              }
              int32_t vabs = abs(v);
              if (vabs > INT16_MAX) { vabs = INT16_MAX; }
              if (dac_offset < vabs) { dac_offset = vabs; }
              else { dac_offset += ((dac_offset < vabs + 128) ? 1 : -1); }
            }
            v += surplus[lr] + dac_offset;
            surplus[lr] = v & 255;
            if (v < 0) { v = 0; }
            else if (v > UINT16_MAX) { v = UINT16_MAX; }
            sound_buf[sound_buf_index ^ 1] = v;
          }
          else if (self->_cfg.buzzer)
          {
            v >>= 8;
            int32_t tmp = surplus[lr];
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
            surplus[lr] = tmp;
            sound_buf[sound_buf_index] = bitdata;
          }
          else
          {
            v += surplus[lr];
            surplus[lr] = v & 255;
            v >>= 8;
            if (v < INT16_MIN) { v = INT16_MIN; }
            else if (v > INT16_MAX) { v = INT16_MAX; }
            sound_buf[sound_buf_index] = v;
          }
          sound_buf_index++;
        } while (++lr != lr_loop);
      } while (sound_buf_index < self->_cfg.dma_buf_len);

      if (!flg_i2s_started)
      {
        flg_i2s_started = true;
        i2s_zero_dma_buffer(self->_cfg.i2s_port);
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

      if (++nodata_count > self->_cfg.dma_buf_count * 2)
      {
        nodata_count = 0;
        flg_i2s_started = false;
        i2s_stop(self->_cfg.i2s_port);
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
        if (self->_cfg.use_dac)
        {
          i2s_set_dac_mode(i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE);
        }
#endif
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
      }
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

    bool res = true;
    if (_cb_set_enabled) { res = _cb_set_enabled(_cb_set_enabled_args, true); }

    res = (ESP_OK == _setup_i2s()) && res;
    if (res)
    {
      size_t stack_size = 1024+(_cfg.dma_buf_len * sizeof(int16_t));
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
    _play_channel_bits = 0;
    for (size_t i = 0; i < sound_channel_max; ++i)
    {
      _ch_info[i].next_wav.clear();
      _ch_info[i].current_wav.clear();
      _ch_info[i].index = 0;
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
      _play_channel_bits &= ~(1 << ch);
      _ch_info[ch].next_wav.clear();
      _ch_info[ch].current_wav.clear();
      _ch_info[ch].index = 0;
    }
  }

  void Speaker_Class::wav_info_t::clear(void)
  {
    sample_rate = 0;
    data = nullptr;
    flg = 0;
    length = 0;
    repeat = 0;
  }

  bool Speaker_Class::tone(float frequency, uint32_t duration, int channel, bool stop_current_sound, const uint8_t* wav_data, size_t array_len, bool stereo)
  {
    return _play_raw(wav_data, array_len, false, false, (int)(frequency * array_len) >> stereo, stereo, (duration != ~0u) ? (duration * frequency / 1000) : ~0u, channel, stop_current_sound, true);
  }

  bool Speaker_Class::_play_raw(const void* wav_data, size_t array_len, bool flg_16bit, bool flg_signed, uint32_t sample_rate, bool flg_stereo, uint32_t repeat_count, int channel, bool stop_current_sound, bool no_clear_index)
  {
    if (!begin() || (_task_handle == nullptr)) { return true; }
    if (array_len == 0) { return true; }
    if ((size_t)channel >= sound_channel_max)
    {
      size_t bits = _play_channel_bits;
      for (int ch = sound_channel_max - 1; ch >= 0; --ch)
      {
        if (0 == ((bits >> ch) & 1))
        {
          channel = ch;
          break;
        }
      }
      if ((size_t)channel >= sound_channel_max) { return true; }
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

    uint8_t chmask = 1 << channel;
    if (!stop_current_sound)
    {
      if (isPlaying(channel) == 2) { return false; }
      while ((_play_channel_bits & chmask) && _ch_info[channel].next_wav.repeat) { taskYIELD(); }
    }
    _ch_info[channel].next_wav = info;
    _play_channel_bits |= chmask;

    xTaskNotifyGive(_task_handle);
    return true;
  }
}
