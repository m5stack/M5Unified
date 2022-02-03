// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Sound_Class.hpp"

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
  static constexpr const size_t dma_buf_len = 128;
  static constexpr const size_t dma_buf_cnt = 8;
  const uint8_t Sound_Class::_default_tone_wav[2] = { 191, 64 };

  int Sound_Class::_calc_rec_rate(void) const
  {
    int rate = (_cfg.mic_sample_rate * _cfg.mic_over_sampling);
    if (_cfg.mic_adc) { rate *= 1.007f; }
    return rate;
  }

  esp_err_t Sound_Class::_setup_i2s(bool mic)
  {
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    i2s_driver_uninstall(_cfg.i2s_port);

    i2s_config_t i2s_config = {
      .mode                 = I2S_MODE_MASTER,
      .sample_rate          = 0,
      .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S_MSB,
      .intr_alloc_flags     = 0,
      .dma_buf_count        = dma_buf_cnt,
      .dma_buf_len          = dma_buf_len,
      .use_apll             = false,
      .tx_desc_auto_clear   = true,
      .fixed_mclk           = 0
    };

    i2s_pin_config_t pin_config = {
      .bck_io_num     = _cfg.pin_bck,
      .ws_io_num      = _cfg.pin_lrck,
      .data_out_num   = _cfg.pin_data_out,
      .data_in_num    = _cfg.pin_data_in,
    };

    if (mic)
    {
      if (_cfg.pin_data_in  < 0) { return ESP_FAIL; }
      i2s_config.sample_rate = _calc_rec_rate();
      i2s_config.mode = _cfg.mic_adc
                      ? (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN )
                      : (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM );
    }
    else
    {
      if (_cfg.pin_data_out < 0) { return ESP_FAIL; }
      i2s_config.sample_rate = _cfg.spk_sample_rate;
      i2s_config.mode = _cfg.spk_dac
                      ? (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN )
                      : (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_TX );
      if (_cfg.spk_dac || _cfg.spk_buzzer)
      {
        pin_config.bck_io_num = I2S_PIN_NO_CHANGE;
        pin_config.ws_io_num  = I2S_PIN_NO_CHANGE;
      }
      if (_cfg.spk_stereo)
      {
        i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
      }
    }

    esp_err_t err = i2s_driver_install(_cfg.i2s_port, &i2s_config, 0, nullptr);
    if (err != ESP_OK) { return err; }

    i2s_zero_dma_buffer(_cfg.i2s_port);

    err = i2s_set_pin(_cfg.i2s_port, &pin_config);
    if (err != ESP_OK) { return err; }

    if (mic)
    {
      if (_cfg.mic_adc)
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
      }
    }
    else
    {
      i2s_dac_mode_t dac_mode = i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE;
      if (_cfg.spk_dac)
      {
        dac_mode = i2s_dac_mode_t::I2S_DAC_CHANNEL_BOTH_EN;
        if (!_cfg.spk_stereo)
        {
          err = i2s_set_dac_mode(i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE);
          dac_mode = (_cfg.pin_data_out == GPIO_NUM_25)
                  ? i2s_dac_mode_t::I2S_DAC_CHANNEL_RIGHT_EN // for GPIO 25
                  : i2s_dac_mode_t::I2S_DAC_CHANNEL_LEFT_EN; // for GPIO 26
        }
        err = i2s_set_dac_mode(dac_mode);
      }
      _dac_mode = dac_mode;
    }

    return err;
#else
    return ESP_FAIL;
#endif
  }

  void Sound_Class::output_task(void* args)
  {
    auto self = (Sound_Class*)args;

    int nodata_count = 0;
    size_t sound_buf_index = 0;
    int16_t sound_buf[dma_buf_len];
    int32_t dac_offset = 0;
    int32_t surplus[2] = { 0, 0 };
    bool flg_i2s_started = false;
    while (self->_task_running)
    {
      int32_t value[2] = { 0, 0 };

      if (self->_play_channel_bits)
      {
        for (size_t idx = 0; idx < sound_channel_max; ++idx)
        {
          if (0 == (self->_play_channel_bits & (1 << idx))) { continue; }
          auto ch_info = &(self->_ch_info[idx]);

          if (ch_info->current_wav.repeat == 0 || ch_info->next_wav.stop_current)
          {
            if (ch_info->next_wav.repeat == 0)
            {
              ch_info->index = 0;
              self->_play_channel_bits &= ~(1 << idx);
              continue;
            }
            if (!ch_info->next_wav.no_clear_index
             || (ch_info->current_wav.data != ch_info->next_wav.data))
            {
              ch_info->index = 0;
            }
            ch_info->current_wav = ch_info->next_wav;
            ch_info->next_wav.clear();
          }

          auto current_wav = &(ch_info->current_wav);
          bool stereo = current_wav->is_stereo;
          int32_t ch_v = ch_info->volume;
          int32_t ch_index = ch_info->index;
          int32_t l, r;
          const void* data = current_wav->data;
          if (current_wav->is_16bit)
          {
            if (current_wav->is_signed)
            {
              auto i16wav = (const int16_t*)data;
              l = i16wav[ch_index         ];
              r = i16wav[ch_index + stereo];
            }
            else
            {
              auto u16wav = (const uint16_t*)data;
              l = (int32_t)u16wav[ch_index         ] + INT16_MIN;
              r = (int32_t)u16wav[ch_index + stereo] + INT16_MIN;
            }
          }
          else
          {
            if (current_wav->is_signed)
            {
              auto i8wav = (const int8_t*)data;
              l = i8wav[ch_index         ];
              r = i8wav[ch_index + stereo];
            }
            else
            {
              auto u8wav = (const uint8_t*)data;
              l = (int32_t)u8wav[ch_index         ] + INT8_MIN;
              r = (int32_t)u8wav[ch_index + stereo] + INT8_MIN;
            }
            ch_v <<= 8;
          }
          value[0] += l * ch_v;
          value[1] += r * ch_v;

          int32_t ch_diff = ch_info->diff + current_wav->sample_rate;
          int32_t spk_sample_rate = self->_cfg.spk_sample_rate;
          if (ch_diff >= spk_sample_rate)
          {
            int32_t tmp = ch_diff / spk_sample_rate;
            ch_diff -= tmp * spk_sample_rate;
            ch_index += tmp * (stereo + 1);

            while (ch_index >= current_wav->length && current_wav->repeat)
            {
              ch_index -= current_wav->length;
              if (ch_info->current_wav.repeat != ~0u) { ch_info->current_wav.repeat--; }
            }
          }
          ch_info->diff = ch_diff;
          ch_info->index = ch_index;
        }
        nodata_count = 0;
      }
      else
      {
        dac_offset = (dac_offset * 255) >> 8;
      }

      {
        size_t loop = 1 + self->_cfg.spk_stereo;
        if (!self->_cfg.spk_stereo)
        {
          int32_t v = (value[0] + value[1]) >> 1; 
          value[0] = v;
          value[1] = v;
        }

        int volume = (self->_master_volume + 1) * (self->_cfg.spk_gain + 1);
        size_t lr = 0;
        do
        {
          int32_t v = ((volume >> 8) * (value[lr] >> 8)) >> 8;
          if (self->_cfg.spk_dac)
          {
            int32_t vabs = abs(v);
            if (dac_offset < vabs) { dac_offset = vabs; }
            else { dac_offset += ((dac_offset < vabs + 128) ? 1 : -1); }
            v += surplus[lr] + dac_offset;
            surplus[lr] = v & 255;
            if (v > UINT16_MAX) { v = UINT16_MAX; }
            sound_buf[sound_buf_index ^ 1] = v;
          }
          else if (self->_cfg.spk_buzzer)
          {
            v += 0x8000;
            int32_t tmp = surplus[lr];
            uint_fast16_t bitdata = 0;
            uint_fast16_t bit = 0x8000;
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
          { /// ESP32のI2Sはビットずれを起こす不具合があるため最上位ビットを使わないよう15bit分を上限とする;
            if (v < -16384) { v = -16384; }
            else if (v > 16383) { v = 16383; }
            sound_buf[sound_buf_index] = v;
          }
          sound_buf_index++;
        } while (++lr != loop);
      }

      if (sound_buf_index >= dma_buf_len)
      {
        sound_buf_index = 0;
        if (!flg_i2s_started)
        {
          flg_i2s_started = true;
          i2s_zero_dma_buffer(self->_cfg.i2s_port);
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
          i2s_set_dac_mode(self->_dac_mode);
#endif
          i2s_start(self->_cfg.i2s_port);
        }
        size_t write_bytes;
        i2s_write(self->_cfg.i2s_port, sound_buf, sizeof(sound_buf), &write_bytes, portMAX_DELAY);

        if (++nodata_count > dma_buf_cnt * 2)
        {
          nodata_count = 0;
          flg_i2s_started = false;
          i2s_stop(self->_cfg.i2s_port);
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
          i2s_set_dac_mode(i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE);
#endif
          ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        }
      }
    }
    i2s_stop(self->_cfg.i2s_port);
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (self->_dac_mode != i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE)
    {
      i2s_set_dac_mode(i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE);
      m5gfx::gpio_lo(self->_cfg.pin_data_out);
      m5gfx::pinMode(self->_cfg.pin_data_out, m5gfx::pin_mode_t::output);
    }
#endif
    self->_sound_task_handle = nullptr;
    vTaskDelete(nullptr);
  }

  void Sound_Class::input_task(void* args)
  {
    auto self = (Sound_Class*)args;

    int oversampling = self->_cfg.mic_over_sampling;
    if (     oversampling < 1) { oversampling = 1; }
    else if (oversampling > 8) { oversampling = 8; }

    int gain = self->_cfg.mic_gain;
    int offset = self->_cfg.mic_offset;
    size_t src_idx = ~0u;
    size_t src_len = 0;
    int16_t src_buf[dma_buf_len];
    int value = 0;
    int os_remain = oversampling;
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

        if (self->_cfg.mic_adc)
        {
          do
          {
            value += (src_buf[src_idx^1] & 0x0FFF) + offset - 2048;
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
        if (gain != oversampling)
        {
          value = value * gain / oversampling;
        }
        if (     value < INT16_MIN) { value = INT16_MIN; }
        else if (value > INT16_MAX) { value = INT16_MAX; }

        if (self->_rec_info[0].is_16bit)
        {
          auto dst = (int16_t*)(self->_rec_info[0].data);
          *dst++ = value;
          self->_rec_info[0].data = dst;
        }
        else
        {
          auto dst = (uint8_t*)(self->_rec_info[0].data);
          *dst++ = (value >> 8) + 128;
          self->_rec_info[0].data = dst;
        }
        value = 0;
        if (--dst_remain == 0) break;
      }
    }
    i2s_stop(self->_cfg.i2s_port);

    self->_sound_task_handle = nullptr;
    vTaskDelete(nullptr);
  }

  bool Sound_Class::setMode(sound_mode_t mode)
  {
    if (_sound_mode == mode && (mode != sound_mode_t::sound_input || _cfg.mic_sample_rate == _rec_sample_rate))
    {
      return true;
    }
    if (_sound_mode == sound_mode_t::sound_output)
    {
      stopPlay();
    }
    if (_sound_task_handle)
    {
      _task_running = false;
      if (_sound_task_handle) { xTaskNotifyGive(_sound_task_handle); }
      do { vTaskDelay(1); } while (_sound_task_handle);
    }

    _sound_mode = mode;

    bool res = true;
    if (_cb_set_mode) { res = _cb_set_mode(_cb_set_mode_args, mode); }
    if (mode == sound_mode_t::sound_off) { return res; }

    bool flg_in = (mode == sound_mode_t::sound_input);
    res = (ESP_OK == _setup_i2s(flg_in)) && res;
    if (res)
    {
      _task_running = true;
      xTaskCreatePinnedToCore(flg_in ? input_task : output_task, "sound_task", 1024, this, _cfg.task_priority, &_sound_task_handle, _cfg.task_pinned_core);
    }

    return res;
  }

  void Sound_Class::stopPlay(void)
  {
    _play_channel_bits = 0;
    for (size_t i = 0; i < sound_channel_max; ++i)
    {
      _ch_info[i].next_wav.clear();
      _ch_info[i].current_wav.clear();
      _ch_info[i].index = 0;
    }
  }

  void Sound_Class::stopPlay(uint8_t ch)
  {
    if ((size_t)ch >= sound_channel_max)
    {
      stopPlay();
    }
    else
    {
      _play_channel_bits &= ~(1 << ch);
      _ch_info[ch].next_wav.clear();
      _ch_info[ch].current_wav.clear();
      _ch_info[ch].index = 0;
    }
  }

  void Sound_Class::wav_info_t::clear(void)
  {
    stop_current = false;
    sample_rate = 0;
    length = 0;
    repeat = 0;
  }

  bool Sound_Class::_set_channel(const Sound_Class::wav_info_t& info, uint8_t channel)
  {
    if (_sound_task_handle == nullptr) { return false; }
    uint8_t chmask = 1 << channel;
    if (!info.stop_current)
    {
      if ((_play_channel_bits & chmask) && _ch_info[channel].next_wav.repeat) { return false; }
    }
    _ch_info[channel].next_wav = info;
    _play_channel_bits |= chmask;

    xTaskNotifyGive(_sound_task_handle);
    return true;
  }

  uint8_t Sound_Class::_autochannel(void)
  {
    for (int ch = sound_channel_max - 1; ch >= 0; --ch)
    {
      if (_ch_info[ch].current_wav.repeat == 0)
      {
        return ch;
      }
    }
    return sound_channel_max;
  }

  bool Sound_Class::tone(float frequency, uint32_t duration, int channel, const uint8_t* wav_data, size_t array_len, bool stereo)
  {
    return _play_raw(wav_data, array_len, false, false, (int)(frequency * array_len) >> stereo, stereo, (duration != ~0u) ? (duration * frequency / 1000) : ~0u, channel, true, true);
  }

  bool Sound_Class::_play_raw(const void* wav_data, size_t array_len, bool flg_16bit, bool flg_signed, uint32_t sample_rate, bool flg_stereo, uint32_t repeat_count, int channel, bool stop_current_sound, bool no_clear_index)
  {
    if (!setMode(sound_mode_t::sound_output)) { return false; }
    if (array_len == 0) { return true; }
    if ((size_t)channel >= sound_channel_max)
    {
      channel = _autochannel(); 
      if ((size_t)channel >= sound_channel_max) { return false; }
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
    return _set_channel(info, channel);
  }

  bool Sound_Class::playRAW(const int8_t* wav, size_t array_len, uint32_t sample_rate, bool stereo, uint32_t repeat_count, int channel, bool stop_current_sound)
  {
    return _play_raw(static_cast<const void* >(wav), array_len, false, true, sample_rate, stereo, repeat_count, channel, stop_current_sound, false);
  }
  bool Sound_Class::playRAW(const uint8_t* wav, size_t array_len, uint32_t sample_rate, bool stereo, uint32_t repeat_count, int channel, bool stop_current_sound)
  {
    return _play_raw(static_cast<const void* >(wav), array_len, false, false, sample_rate, stereo, repeat_count, channel, stop_current_sound, false);
  }
  bool Sound_Class::playRAW(const int16_t* wav, size_t array_len, uint32_t sample_rate, bool stereo, uint32_t repeat_count, int channel, bool stop_current_sound)
  {
    return _play_raw(static_cast<const void* >(wav), array_len, true, true, sample_rate, stereo, repeat_count, channel, stop_current_sound, false);
  }

  bool Sound_Class::_rec_raw(void* recdata, size_t array_len, bool flg_16bit, uint32_t sample_rate)
  {
    if (array_len == 0) { return true; }
    recording_info_t info;
    info.data = recdata;
    info.length = array_len;
    info.is_16bit = flg_16bit;

    if (_rec_sample_rate != sample_rate)
    {
      _rec_sample_rate = sample_rate;
      while (isRecording()) { vTaskDelay(1); }
      setMode(sound_mode_t::sound_off);
    }
    _cfg.mic_sample_rate = sample_rate;

    if (!setMode(sound_mode_t::sound_input)) { return false; }
    while (_rec_info[1].length) { taskYIELD(); }
    _rec_info[1] = info;
    _is_recording = true;
    xTaskNotifyGive(this->_sound_task_handle);
    return true;
  }

  bool Sound_Class::record(uint8_t* recdata, size_t array_len, uint32_t sample_rate)
  {
    return _rec_raw(recdata, array_len, false, sample_rate);
  }

  bool Sound_Class::record(int16_t* recdata, size_t array_len, uint32_t sample_rate)
  {
    return _rec_raw(recdata, array_len, true, sample_rate);
  }
}
