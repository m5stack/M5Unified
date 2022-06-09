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

  const uint8_t Speaker_Class::_default_tone_wav[16] = { 177, 219, 246, 255, 246, 219, 177, 128, 79, 37, 10, 1, 10, 37, 79, 128 }; // sin wave data

  esp_err_t Speaker_Class::_setup_i2s(void)
  {
    if (_cfg.pin_data_out < 0) { return ESP_FAIL; }

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    /// DACが使用できるのはI2Sポート0のみ。;
    if (_cfg.use_dac && _cfg.i2s_port != I2S_NUM_0) { return ESP_FAIL; }
#endif

    i2s_config_t i2s_config;
    memset(&i2s_config, 0, sizeof(i2s_config_t));
    i2s_config.mode                 = (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_TX );
    i2s_config.sample_rate          = 48000; // dummy setting
    i2s_config.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format       = _cfg.stereo || _cfg.buzzer
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

    esp_err_t err;
    if (ESP_OK != (err = i2s_driver_install(_cfg.i2s_port, &i2s_config, 0, nullptr)))
    {
      i2s_driver_uninstall(_cfg.i2s_port);
      err = i2s_driver_install(_cfg.i2s_port, &i2s_config, 0, nullptr);
    }
    if (err != ESP_OK) { return err; }

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
      { /// レジスタを操作してDACモードの設定を有効にする(I2S0のみ。I2S1はDAC,ADC非対応) ;
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

  static void calcClockDiv(uint32_t* div_a, uint32_t* div_b, uint32_t* div_n, uint32_t baseClock, uint32_t targetFreq)
  {
    if (baseClock <= targetFreq << 1)
    { /// Nは最小2のため、基準クロックが目標クロックの2倍より小さい場合は値を確定する;
      *div_n = 2;
      *div_a = 1;
      *div_b = 0;
      return;
    }
    uint32_t save_n = 255;
    uint32_t save_a = 63;
    uint32_t save_b = 62;
    if (targetFreq)
    {
      float fdiv = (float)baseClock / targetFreq;
      uint32_t n = (uint32_t)fdiv;
      if (n < 256)
      {
        fdiv -= n;

        float check_base = baseClock;
// 探索時の誤差を少なくするため、値を大きくしておく;
        while ((int32_t)targetFreq >= 0) { targetFreq <<= 1; check_base *= 2; }
        float check_target = targetFreq;

        uint32_t save_diff = UINT32_MAX;
        if (n < 255)
        { /// 初期値の設定はNがひとつ上のものを設定しておく;
          save_a = 1;
          save_b = 0;
          save_n = n + 1;
          save_diff = abs((int)(check_target - check_base / (float)save_n));
        }

        for (uint32_t a = 1; a < 64; ++a)
        {
          uint32_t b = roundf(a * fdiv);
          if (a <= b) { continue; }
          uint32_t diff = abs((int)(check_target - ((check_base * a) / (n * a + b))));
          if (save_diff <= diff) { continue; }
          save_diff = diff;
          save_a = a;
          save_b = b;
          save_n = n;
          if (!diff) { break; }
        }
      }
    }
    *div_n = save_n;
    *div_a = save_a;
    *div_b = save_b;
  }

/// レート変換係数 (実際に設定されるレートが浮動小数になる場合があるため、入力と出力の両方のサンプリングレートに係数を掛け、誤差を減らす);
  #define SAMPLERATE_MUL 256

  void Speaker_Class::spk_task(void* args)
  {
    auto self = (Speaker_Class*)args;
    const i2s_port_t i2s_port = self->_cfg.i2s_port;
    const bool out_stereo = self->_cfg.stereo;

    static constexpr uint32_t base = 160*1000*1000; // 160 MHz
    uint32_t bits = (self->_cfg.use_dac) ? 2 : 32; /// 1サンプリング当たりの出力ビット数 (DACは2ch分の出力で2回、I2Sは16bit x2chで32回);
    uint32_t div_a, div_b, div_n;
    uint32_t div_m = 64 / bits; /// MCLKを使用しないので、サンプリングレート誤差が少なくなるようにdiv_mを調整する;
    // MCLKを使用するデバイスに対応する場合には、div_mを使用してBCKとMCKの比率を調整する;

    calcClockDiv(&div_a, &div_b, &div_n, base, div_m * bits * self->_cfg.sample_rate);

    /// 実際に設定されたサンプリングレートの算出を行う;
    const int32_t spk_sample_rate_x256 = (float)base * SAMPLERATE_MUL / ((float)(div_b * div_m * bits) / (float)div_a + (div_n * div_m * bits));
//  ESP_EARLY_LOGW("Speaker_Class", "sample rate:%d Hz = %d MHz/(%d+(%d/%d))/%d/%d = %d Hz", self->_cfg.sample_rate, base / 1000000, div_n, div_b, div_a, div_m, bits, spk_sample_rate_x256 / SAMPLERATE_MUL);

#if defined ( CONFIG_IDF_TARGET_ESP32C3 )

    auto dev = &I2S0;
    dev->tx_conf1.tx_bck_div_num = div_m - 1;
    dev->tx_clkm_conf.tx_clkm_div_num = div_n;
    bool yn1 = (div_b > (div_a >> 1)) ? 1 : 0;

    dev->tx_clkm_div_conf.val = 0;
    if (yn1) {
      dev->tx_clkm_div_conf.tx_clkm_div_yn1 = 1;
      div_b = div_a - div_b;
    }
    if (div_b)
    {
      dev->tx_clkm_div_conf.tx_clkm_div_z = div_b;
      dev->tx_clkm_div_conf.tx_clkm_div_y = div_a % div_b;
      dev->tx_clkm_div_conf.tx_clkm_div_x = div_a / div_b - 1;
    }
    dev->tx_clkm_conf.tx_clk_sel = 2;   // PLL_160M_CLK
    dev->tx_clkm_conf.clk_en = 1;
    dev->tx_clkm_conf.tx_clk_active = 1;

#else

    auto dev = (i2s_port == i2s_port_t::I2S_NUM_1) ? &I2S1 : &I2S0;

    dev->sample_rate_conf.tx_bck_div_num = div_m;
    dev->clkm_conf.clkm_div_a = div_a;
    dev->clkm_conf.clkm_div_b = div_b;
    dev->clkm_conf.clkm_div_num = div_n;
    dev->clkm_conf.clka_en = 0; // APLL disable : PLL_160M

#endif

    const float magnification = (float)self->_cfg.magnification / spk_sample_rate_x256
                              / (~0u >> ((self->_cfg.use_dac || self->_cfg.buzzer) ? 0 : 8));
    const size_t dma_buf_len = self->_cfg.dma_buf_len & ~1;
    int nodata_count = 0;
    int32_t dac_offset = self->_cfg.dac_zero_level << 8;
    bool flg_i2s_started = false;

    union
    {
      int16_t surplus16 = 0;
      uint8_t surplus[2];
    };

    union
    {
      float* float_buf;
      int16_t* sound_buf;
      int32_t* sound_buf32;
    };
    float_buf = (float*)alloca(dma_buf_len * sizeof(float));

    while (self->_task_running)
    {
      if (nodata_count)
      {
        if (nodata_count > self->_cfg.dma_buf_count)
        {
          nodata_count = 0;
          flg_i2s_started = false;
          i2s_stop(i2s_port);
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
          if (self->_cfg.use_dac)
          {
            i2s_set_dac_mode(i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE);
          }
#endif
          ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
          continue;
        }
        ulTaskNotifyTake( pdTRUE, 0 );
      }

      memset(float_buf, 0, dma_buf_len * sizeof(float));
      ++nodata_count;

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
        i2s_zero_dma_buffer(i2s_port);
        i2s_start(i2s_port);
        if (self->_cfg.use_dac && self->_cfg.dac_zero_level != 0)
        {
          size_t idx = 0;
          do
          {
            sound_buf[idx^1] = dac_offset * idx / dma_buf_len;
          } while (++idx < dma_buf_len);
          size_t write_bytes;
          i2s_write(i2s_port, sound_buf, dma_buf_len * sizeof(int16_t), &write_bytes, portMAX_DELAY);
          memset(float_buf, 0, dma_buf_len * sizeof(float));
        }
      }

      float volume = magnification * (self->_master_volume * self->_master_volume);

      for (size_t ch = 0; ch < sound_channel_max; ++ch)
      {
        if (0 == (self->_play_channel_bits.load() & (1 << ch))) { continue; }
        nodata_count = 0;

        auto ch_info = &(self->_ch_info[ch]);
        wav_info_t* current_wav = &(ch_info->wavinfo[!ch_info->flip]);
        wav_info_t* next_wav    = &(ch_info->wavinfo[ ch_info->flip]);

        size_t idx = 0;

        if (current_wav->repeat == 0 || next_wav->stop_current)
        {
label_next_wav:
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
            if (current_wav->repeat == 0)
            {
              self->_play_channel_bits.fetch_and(~(1 << ch));
              if (current_wav->repeat == 0)
              {
                ch_info->diff = 0;
                continue;
              }
              self->_play_channel_bits.fetch_or(1 << ch);
            }
          }
        }
        const void* data = current_wav->data;
        const bool in_stereo = current_wav->is_stereo;
        const int32_t in_rate = current_wav->sample_rate_x256;
        int32_t tmp = ch_info->volume;
        tmp *= tmp;
        if (!current_wav->is_16bit) { tmp <<= 8; }
        if (self->_cfg.stereo) { tmp <<= 1; }
        const float ch_v = volume * tmp;

        bool liner_flip = ch_info->liner_flip;
        auto liner_base = ch_info->liner_buf[ liner_flip];
        auto liner_prev = ch_info->liner_buf[!liner_flip];

        int ch_diff = ch_info->diff;
        if (ch_diff < 0) { goto label_continue_sample; }

        do
        {
          {
            size_t ch_index = ch_info->index;
            do
            {
              if (ch_index >= current_wav->length)
              {
                ch_index -= current_wav->length;
                auto repeat = current_wav->repeat;
                if (repeat != ~0u)
                {
                  current_wav->repeat = --repeat;
                  if (repeat == 0)
                  {
                    ch_info->index = ch_index;
                    ch_info->liner_flip = (liner_prev < liner_base);

                    goto label_next_wav;
                  }
                }
              }

              int32_t l, r;
              if (current_wav->is_16bit)
              {
                auto wav = (const int16_t*)data;
                l = wav[ch_index + in_stereo];
                r = wav[ch_index];
                ch_index += in_stereo + 1;
                if (!current_wav->is_signed)
                {
                  l = (l & 0xFFFF) + INT16_MIN;
                  r = (r & 0xFFFF) + INT16_MIN;
                }
              }
              else
              {
                auto wav = (const uint8_t*)data;
                l = wav[ch_index + in_stereo];
                r = wav[ch_index];
                ch_index += in_stereo + 1;
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
              std::swap(liner_base, liner_prev);

              if (!out_stereo) { l += r; }
              else
              {
                liner_base[1] = r * ch_v;
              }
              liner_base[0] = l * ch_v;

              ch_diff -= spk_sample_rate_x256;
            } while (ch_diff >= 0);
            ch_info->index = ch_index;
          }

label_continue_sample:

/// liner_prevからliner_baseへの２サンプル間の線形補間;
          float base_l = liner_base[0];
          float step_l = base_l - liner_prev[0];
          base_l *= spk_sample_rate_x256;
          base_l += step_l * ch_diff;
          step_l *= in_rate;
          if (out_stereo)
          {
            float base_r = liner_base[1];
            float step_r = base_r - liner_prev[1];
            base_r *= spk_sample_rate_x256;
            base_r += step_r * ch_diff;
            step_r *= in_rate;
            do
            {
              float_buf[  idx] += base_l;
              float_buf[++idx] += base_r;
              base_l += step_l;
              base_r += step_r;
              ch_diff += in_rate;
            } while (++idx < dma_buf_len && ch_diff < 0);
          }
          else
          {
            do
            {
              float_buf[idx] += base_l;
              base_l += step_l;
              ch_diff += in_rate;
            } while (++idx < dma_buf_len && ch_diff < 0);
          }
          ch_info->diff = ch_diff;
        } while (idx < dma_buf_len);
        ch_info->liner_flip = (liner_prev < liner_base);
      }

      if (self->_cfg.use_dac)
      {
/// DAC出力は cfg.dac_zero_levelが0に設定されている場合、振幅のオフセットを動的に変更する。;
/// これはESP32のDAC出力は高ければ高いほどノイズも増加するため、なるべく低いDAC出力を用いてノイズを低減することを目的とする。;
        const bool zero_bias = (self->_cfg.dac_zero_level == 0);
        if (nodata_count == 0)
        {
          if (zero_bias) { dac_offset -= (dac_offset * dma_buf_len) >> 15; }
          size_t idx = 0;
          do
          {
            int32_t v = float_buf[idx];
            if (zero_bias)
            {
              int32_t vabs = abs(v);
              if (dac_offset < vabs)
              {
                dac_offset = (INT16_MAX < vabs) ? INT16_MAX : vabs;
              }
            }
            v += dac_offset;
            auto s = &surplus[idx & out_stereo];
            v += *s;
            *s = v;

            if (v < 0) { v = 0; }
            else if (v > UINT16_MAX) { v = UINT16_MAX; }
            sound_buf[idx ^ 1] = v;
          } while (++idx < dma_buf_len);
        }
        else
        {
          if (nodata_count == 1)
          {
            surplus16 = dac_offset;
          }
          uint_fast16_t offset = surplus16;
          if (offset)
          {
            nodata_count = 1;
            if (--offset > 16)
            {
              size_t idx = 0;
              do
              {
                offset = (offset * 255) >> 8;
                sound_buf[idx] = offset;
              } while (++idx < dma_buf_len);
              if (offset <= 16) { offset = 16; }
            }
            surplus16 = offset;
            if (zero_bias) { dac_offset = offset; }
          }
        }
      }
      else if (self->_cfg.buzzer)
      {
/// ブザー出力は 1bit ΔΣ方式。 I2Sデータ出力をブザーの駆動信号として利用する;
/// 出力はモノラル限定だが、I2Sへはステレオ扱いで出力する。;
/// (I2Sをモノラル設定にした場合は同じデータが２チャンネル分送信されてしまうため、敢えてステレオ扱いとしている);
        int32_t tmp = (uint16_t)surplus16;
        size_t idx = 0;
        do
        {
          int32_t v = float_buf[idx];
          v = INT16_MIN - v;
          uint32_t bitdata = 0;
          uint32_t bit = 0x80000000;
          do
          {
            if ((tmp += v) < 0)
            {
              tmp += 0x10000;
              bitdata |= bit;
            }
          } while (bit >>= 1);
          sound_buf32[idx] = bitdata;
        } while (++idx < dma_buf_len);
        surplus16 = nodata_count ? 0x8000 : tmp;
      }
      else
      {
        size_t idx = 0;
        do
        {
          int32_t v = float_buf[idx];
          v += surplus[idx & out_stereo];
          surplus[idx & out_stereo] = v;
          v >>= 8;
          if (v < INT16_MIN) { v = INT16_MIN; }
          else if (v > INT16_MAX) { v = INT16_MAX; }
          sound_buf[idx ^ 1] = v;
        } while (++idx < dma_buf_len);
      }

      size_t write_bytes;
      i2s_write(i2s_port, sound_buf, dma_buf_len * sizeof(int16_t) << self->_cfg.buzzer, &write_bytes, portMAX_DELAY);
    }
    i2s_stop(i2s_port);
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
#if portNUM_PROCESSORS > 1
      if (((size_t)_cfg.task_pinned_core) < portNUM_PROCESSORS)
      {
        xTaskCreatePinnedToCore(spk_task, "spk_task", stack_size, this, _cfg.task_priority, &_task_handle, _cfg.task_pinned_core);
      }
      else
#endif
      {
        xTaskCreate(spk_task, "spk_task", stack_size, this, _cfg.task_priority, &_task_handle);
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
    sample_rate_x256 = 0;
    flg = 0;
    repeat = 0;
  }

  bool Speaker_Class::_set_next_wav(size_t ch, const wav_info_t& wav)
  {
    auto chinfo = &_ch_info[ch];
    uint8_t chmask = 1 << ch;
    if (!wav.stop_current)
    {
      while ((_play_channel_bits.load() & chmask) && (chinfo->wavinfo[chinfo->flip].repeat))
      {
        if (chinfo->wavinfo[!chinfo->flip].repeat == ~0u) { return false; }
        xSemaphoreTake(_task_semaphore, 1);
      }
    }
    chinfo->wavinfo[chinfo->flip] = wav;
    _play_channel_bits.fetch_or(chmask);

    xTaskNotifyGive(_task_handle);
    return true;
  }

  bool Speaker_Class::_play_raw(const void* data, size_t array_len, bool flg_16bit, bool flg_signed, float sample_rate, bool flg_stereo, uint32_t repeat_count, int channel, bool stop_current_sound, bool no_clear_index)
  {
    if (!begin() || (_task_handle == nullptr)) { return true; }
    if (array_len == 0 || data == nullptr) { return true; }
    size_t ch = (size_t)channel;
    if (ch >= sound_channel_max)
    {
      size_t bits = _play_channel_bits.load();
      for (ch = sound_channel_max - 1; ch < sound_channel_max; --ch)
      {
        if (0 == ((bits >> ch) & 1)) { break; }
      }
      if (ch >= sound_channel_max) { return false; }
    }
    wav_info_t info;
    info.data = data;
    info.length = array_len;
    info.repeat = repeat_count ? repeat_count : ~0u;
    info.sample_rate_x256 = sample_rate * SAMPLERATE_MUL;
    info.is_stereo = flg_stereo;
    info.is_16bit = flg_16bit;
    info.is_signed = flg_signed;
    info.stop_current = stop_current_sound;
    info.no_clear_index = no_clear_index;

    return _set_next_wav(ch, info);
  }

  bool Speaker_Class::playWav(const uint8_t* wav_data, size_t data_len, uint32_t repeat, int channel, bool stop_current_sound)
  {
    struct __attribute__((packed)) wav_header_t
    {
      char RIFF[4];
      uint32_t chunk_size;
      char WAVEfmt[8];
      uint32_t fmt_chunk_size;
      uint16_t audiofmt;
      uint16_t channel;
      uint32_t sample_rate;
      uint32_t byte_per_sec;
      uint16_t block_size;
      uint16_t bit_per_sample;
    };
    struct __attribute__((packed)) sub_chunk_t
    {
      char identifier[4];
      uint32_t chunk_size;
      uint8_t data[1];
    };

    auto wav = (wav_header_t*)wav_data;
    /*
    ESP_LOGD("wav", "RIFF           : %.4s" , wav->RIFF          );
    ESP_LOGD("wav", "chunk_size     : %d"   , wav->chunk_size    );
    ESP_LOGD("wav", "WAVEfmt        : %.8s" , wav->WAVEfmt       );
    ESP_LOGD("wav", "fmt_chunk_size : %d"   , wav->fmt_chunk_size);
    ESP_LOGD("wav", "audiofmt       : %d"   , wav->audiofmt      );
    ESP_LOGD("wav", "channel        : %d"   , wav->channel       );
    ESP_LOGD("wav", "sample_rate    : %d"   , wav->sample_rate   );
    ESP_LOGD("wav", "byte_per_sec   : %d"   , wav->byte_per_sec  );
    ESP_LOGD("wav", "block_size     : %d"   , wav->block_size    );
    ESP_LOGD("wav", "bit_per_sample : %d"   , wav->bit_per_sample);
    */
    if ( !wav_data
      || memcmp(wav->RIFF,    "RIFF",     4)
      || memcmp(wav->WAVEfmt, "WAVEfmt ", 8)
      || wav->audiofmt != 1
      || wav->bit_per_sample < 8
      || wav->bit_per_sample > 16
      || wav->channel == 0
      || wav->channel > 2
      )
    {
      return false;
    }

    sub_chunk_t* sub = (sub_chunk_t*)(wav_data + offsetof(wav_header_t, audiofmt) + wav->fmt_chunk_size);
    /*
    ESP_LOGD("wav", "sub id         : %.4s" , sub->identifier);
    ESP_LOGD("wav", "sub chunk_size : %d"   , sub->chunk_size);
    */
    while(memcmp(sub->identifier, "data", 4) && (uint8_t*)sub < wav_data + wav->chunk_size + 8)
    {
      sub = (sub_chunk_t*)((uint8_t*)sub + offsetof(sub_chunk_t, data) + sub->chunk_size);
      /*
      ESP_LOGD("wav", "sub id         : %.4s" , sub->identifier);
      ESP_LOGD("wav", "sub chunk_size : %d"   , sub->chunk_size);
      */
    }
    if (memcmp(sub->identifier, "data", 4))
    {
      return false;
    }

    data_len = data_len > sizeof(wav_header_t) ? data_len - sizeof(wav_header_t) : 0;
    if (data_len > sub->chunk_size) { data_len = sub->chunk_size; }
    bool flg_16bit = (wav->bit_per_sample >> 4);
    return _play_raw( sub->data
                    , data_len >> flg_16bit
                    , flg_16bit
                    , flg_16bit
                    , wav->sample_rate
                    , wav->channel > 1
                    , repeat
                    , channel
                    , stop_current_sound
                    , false
                    );
  }

}
