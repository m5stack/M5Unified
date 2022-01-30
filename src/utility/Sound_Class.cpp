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
  const size_t Sound_Class::dma_buf_len;
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
      if (_cfg.spk_stereo) // || _cfg.spk_buzzer)
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

  esp_err_t Sound_Class::_write_i2s(int32_t* value, int32_t volume)
  {
    size_t lr = 0;
    size_t loop = 2;
    if (!_cfg.spk_stereo)
    {
      loop = 1;
      int32_t v = (value[0] + value[1]) >> 1; 
      value[0] = v;
      value[1] = v;
    }

    do
    {
      value[lr] = (volume >> 4) * (value[lr] >> 4) >> 8;
    } while (++lr != loop);

    if (_cfg.spk_dac || _cfg.spk_buzzer)
    {
      int32_t offset = _dac_offset * 255 >> 8;
      volume = abs(value[0]);
      if (_cfg.spk_stereo) { volume = std::max<int32_t>(volume, abs(value[1])); }
      if (offset < volume) { offset = volume; }
      volume = offset;
      _dac_offset = offset;
    }

    lr = 0;
    do
    {
      int32_t v = value[lr];
      if (_cfg.spk_dac)
      {
        v += _surplus[lr] + volume;
        int32_t surplus = v & 255;
        _surplus[lr] = surplus;
        v -= surplus;
        if (v < 0) { v = 0; }
        else if (v > 65535) { v = 65535; }
        _sound_buf[_sound_buf_index ^ 1] = v;
      }
      else if (_cfg.spk_buzzer)
      {
        v += _surplus[lr] + ((UINT16_MAX - 3855) >> 1); // (volume >> 1);
        // v += _surplus[lr] + ((UINT16_MAX - 3855 + volume) >> 2);
        uint32_t surplus = v % 3855;
        _surplus[lr] = surplus;
        v -= surplus;
        if (v < 0) { v = 0; }
        else if (v > 65534) { v = 65534; } // (65535 / 3855 == 17) 演算結果を16以下にするために意図的に上限65534とする ;
        static constexpr const uint16_t tbl[17] = 
        {
          0b0000000000000000, //  0
          0b0000000010000000, //  1
          0b0000100000001000, //  2
          0b0001000010000100, //  3
          0b0010001000100010, //  4
          0b0010010010010010, //  5
          0b0100100101001001, //  6
          0b0101001010100101, //  7
          0b0101010101010101, //  8
          0b0101101010101101, //  9
          0b0110101101101011, // 10
          0b0110110111011011, // 11
          0b0111011101110111, // 12
          0b0111101111101111, // 13
          0b0111111101111111, // 14
          0b0111111111111111, // 15
          0b1111111111111111, // 16
        };
        _sound_buf[_sound_buf_index] = tbl[v / 3855];
      }
      else
      {
        if (v < -16384) { v = -16384; }
        else if (v > 16383) { v = 16383; }
        _sound_buf[_sound_buf_index] = v;
      }
      _sound_buf_index++;
    } while (++lr != loop);
    if (_sound_buf_index >= dma_buf_len)
    {
      _sound_buf_index = 0;
      size_t write_bytes;
      return i2s_write(_cfg.i2s_port, _sound_buf, sizeof(_sound_buf), &write_bytes, portMAX_DELAY);
    }
    else
    {
      return 1;
    }
  }

  void Sound_Class::output_task(void* args)
  {
    auto self = (Sound_Class*)args;

    int volume = 0;
    int nodata_count = 0;
    while (self->_task_running)
    {
      int32_t value[2] = { 0, 0 };

      for (size_t idx = 0; idx < sound_channel_max; ++idx)
      {
        if (0 == (self->_play_channel_bits & (1 << idx))) { continue; }
        auto ch_info = &(self->_ch_info[idx]);
        // if (!wav->enabled) { continue; }

        if (ch_info->current_wav.repeat == 0 || ch_info->next_wav.stop_current)
        {
          if (ch_info->next_wav.repeat == 0)
          {
            // wav->enabled = false;
            self->_play_channel_bits &= ~(1 << idx);
            ch_info->index = 0;
            continue;
          }
          ch_info->current_wav = ch_info->next_wav;
          ch_info->next_wav.clear();
        }

        auto current_wav = &(ch_info->current_wav);
        bool stereo = current_wav->is_stereo;
        size_t ch_index = ch_info->index;
        if (current_wav->is_16bit)
        {
          auto i16wav = (const int16_t*)current_wav->data;
          value[0] += (int32_t)(i16wav[ch_index         ] * ch_info->volume) >> 8;
          value[1] += (int32_t)(i16wav[ch_index + stereo] * ch_info->volume) >> 8;
        }
        else
        {
          auto u8wav = (const uint8_t*)current_wav->data;
          value[0] += ((int32_t)u8wav[ch_index         ] - 128) * ch_info->volume;
          value[1] += ((int32_t)u8wav[ch_index + stereo] - 128) * ch_info->volume;
        }
        ch_info->diff += current_wav->sample_rate;
        if (ch_info->diff >= self->_cfg.spk_sample_rate)
        {
          while (ch_info->diff >= self->_cfg.spk_sample_rate)
          {
            ch_info->diff -= self->_cfg.spk_sample_rate;
            ch_index += 1 + stereo;
          }

          while (ch_index >= current_wav->length && ch_info->current_wav.repeat)
          {
            ch_index -= current_wav->length;
            if (ch_info->current_wav.repeat != ~0u) { ch_info->current_wav.repeat--; }
          }
        }
        ch_info->index = ch_index;
      }
      if (self->_play_channel_bits)
      {
        // volume = (volume + (self->_master_volume) * (self->_cfg.spk_gain + 1)) >> 1;
        volume = (self->_master_volume) * (self->_cfg.spk_gain + 1);
      }
      else
      {
        if (volume) { volume = (volume * 255) >> 8; }
        else
        {
          if (++nodata_count > (dma_buf_len * dma_buf_cnt))
          {
            nodata_count = 0;
            i2s_stop(self->_cfg.i2s_port);
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
            auto dac_mode = self->_dac_mode;
            if (dac_mode != i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE)
            {
              i2s_set_dac_mode(i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE);
              ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
              i2s_set_dac_mode(dac_mode);
            }
            else
#endif
            {
              ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
            }
            i2s_start(self->_cfg.i2s_port);
          }
        }
      }
      self->_write_i2s(value, volume);
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
    int16_t* src_buf = self->_sound_buf;
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
        // self->_rec_info[0].length = dst_remain;
      }
    }
    i2s_stop(self->_cfg.i2s_port);

    self->_sound_task_handle = nullptr;
    vTaskDelete(nullptr);
  }

  bool Sound_Class::_set_mode(sound_mode_t mode)
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

    bool res = true;
    if (_cb_set_mode) { res = _cb_set_mode(_cb_set_mode_args, mode); }

    _sound_mode = mode;
    _sound_buf_index = 0;
    if (mode == sound_mode_t::sound_off) { return res; }

    bool flg_in = (mode == sound_mode_t::sound_input);
    res = (ESP_OK == _setup_i2s(flg_in)) && res;
    if (res)
    {
      _task_running = true;
      xTaskCreatePinnedToCore(flg_in ? input_task : output_task, "sound_task", 2048, this, _cfg.task_priority, &_sound_task_handle, _cfg.task_pinned_core);
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
      while ((_play_channel_bits & chmask) && _ch_info[channel].next_wav.repeat) { vTaskDelay(1); }
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
    for (int ch = sound_channel_max - 1; ch >= 0; --ch)
    {
      if (_ch_info[ch].next_wav.repeat == 0
       && _ch_info[ch].current_wav.repeat != ~0u)
      {
        return ch;
      }
    }
    return sound_channel_max;
  }

  bool Sound_Class::tone(float Hz, uint32_t msec, int channel, const uint8_t* wav_data, size_t array_len, bool stereo)
  {
    if (array_len == 0) { return false; }
    if ((size_t)channel >= sound_channel_max) { channel = _autochannel(); }
    if ((size_t)channel >= sound_channel_max) { return false; }
    if (!_set_mode(sound_mode_t::sound_output)) { return false; }

    wav_info_t info;
    info.data = wav_data;
    info.length = array_len;
    info.repeat = msec != ~0u ? msec * Hz / 1000 : ~0u;
    info.sample_rate = (int)(Hz * array_len) >> stereo;
    info.is_stereo = stereo;
    info.is_16bit = false;
    info.stop_current = true;
    return _set_channel(info, channel);
  }

  bool Sound_Class::_play_raw(const void* wav_data, size_t wav_len, bool flg_16bit, bool flg_stereo, uint32_t sample_rate, uint32_t repeat_count, uint8_t channel)
  {
    if (wav_len == 0) { return false; }
    if ((size_t)channel >= sound_channel_max) { channel = _autochannel(); }
    if ((size_t)channel >= sound_channel_max) { return false; }
    if (!_set_mode(sound_mode_t::sound_output)) { return false; }
    wav_info_t info;
    info.data = wav_data;
    info.length = wav_len;
    info.repeat = repeat_count ? repeat_count : ~0u;
    info.sample_rate = sample_rate;
    info.is_stereo = flg_stereo;
    info.is_16bit = flg_16bit;
    info.stop_current = false;
    return _set_channel(info, channel);
  }

  bool Sound_Class::playRAW(const uint8_t* wav, size_t array_len, bool stereo, uint32_t sample_rate, uint32_t repeat_count, int channel)
  {
    return _play_raw(static_cast<const void* >(wav), array_len, false, stereo, sample_rate, repeat_count, channel);
  }
  bool Sound_Class::playRAW(const int16_t* wav, size_t array_len, bool stereo, uint32_t sample_rate, uint32_t repeat_count, int channel)
  {
    return _play_raw(static_cast<const void* >(wav), array_len, true, stereo, sample_rate, repeat_count, channel);
  }

  bool Sound_Class::_rec_raw(void* recdata, size_t array_len, bool flg_16bit, uint32_t sample_rate)
  {
    recording_info_t info;
    info.data = recdata;
    info.length = array_len;
    info.is_16bit = flg_16bit;

    if (_rec_sample_rate != sample_rate)
    {
      _rec_sample_rate = sample_rate;
      while (isRecording()) { vTaskDelay(1); }
      _set_mode(sound_mode_t::sound_off);
    }
    _cfg.mic_sample_rate = sample_rate;

    if (!_set_mode(sound_mode_t::sound_input)) { return false; }
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
