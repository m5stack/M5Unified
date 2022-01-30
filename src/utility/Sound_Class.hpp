// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_Sound_Class_H__
#define __M5_Sound_Class_H__

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2s.h>

namespace m5
{
  class M5Unified;

  enum sound_mode_t
  {
    sound_off,
    sound_output,
    sound_input,
  };

  struct sound_config_t
  {
    /// i2s_data_out (for spk)
    int pin_data_out = -1;

    /// i2s_data_in (for mic)
    int pin_data_in = -1;

    /// i2s_bck
    int pin_bck = -1;

    /// i2s_lrck
    int pin_lrck = -1;

    int spk_sample_rate = 64000;

    /// use stereo output
    bool spk_stereo = false;

    /// use single gpio buzzer ( need only pin_data_out )
    bool spk_buzzer = false;

    /// use DAC speaker ( need only pin_data_out = 25 or 26 )
    bool spk_dac = false;

    /// multiplier for output value
    uint8_t spk_gain = 128;

    /// use analog input mic ( need only pin_data_in )
    bool mic_adc = false;

    /// offset correction value of ADC input value
    int mic_offset = 0;

    uint8_t mic_over_sampling = 4;

    int mic_sample_rate = 16000;

    /// multiplier for input value
    uint8_t mic_gain = 10;

    int task_priority = 2;
    int task_pinned_core = 0;
    i2s_port_t i2s_port = i2s_port_t::I2S_NUM_0;
  };


  class Sound_Class
  {
  friend M5Unified;

    static constexpr const size_t sound_channel_max = 8;
    static constexpr const size_t dma_buf_len = 64;
    static constexpr const size_t dma_buf_cnt = 10;

    static const uint8_t _default_tone_wav[2];

  public:

    const sound_config_t& config(void) const { return _cfg; }
    void config(const sound_config_t& cfg) { _cfg = cfg; }

    bool hasSpk(void) const { return _cfg.pin_data_out >= 0; }
    bool hasMic(void) const { return _cfg.pin_data_in  >= 0; }

    /// play simple tone sound.
    /// @param frequency tone frequency (Hz)
    /// @param duration tone duration (msec)
    /// @param channel virtual channel number. Up to 8, (default = automatically selected)
    /// @param wav_data Single amplitude audio data. 8bit unsigned wav.
    /// @param array_len size of wav_data.
    /// @param stereo true=data is stereo / false=data is mono.
    bool tone(float frequency, uint32_t duration, int channel, const uint8_t* wav_data, size_t array_len, bool stereo = false);

    /// play simple tone sound.
    /// @param frequency tone frequency (Hz)
    /// @param duration tone duration (msec)
    /// @param channel virtual channel number. Up to 8, (default = automatically selected)
    bool tone(float frequency, uint32_t duration = ~0u, int channel = -1) { return tone(frequency, duration, channel, _default_tone_wav, sizeof(_default_tone_wav), false); }

    /// for unsigned 8bit wav data
    bool playRAW(const uint8_t* wav_data, size_t array_len, bool stereo = false, uint32_t sample_rate = 44100, uint32_t repeat_count = 1, int channel = -1);

    /// for signed 16bit wav data
    bool playRAW(const int16_t* wav_data, size_t array_len, bool stereo = false, uint32_t sample_rate = 44100, uint32_t repeat_count = 1, int channel = -1);


    void setRecordSampleRate(uint32_t sample_rate) { _cfg.mic_sample_rate = sample_rate; }

    bool record(uint8_t* recdata, size_t array_len) { return record(recdata, array_len, _cfg.mic_sample_rate); }
    bool record(int16_t* recdata, size_t array_len) { return record(recdata, array_len, _cfg.mic_sample_rate); }
    bool record(uint8_t* recdata, size_t array_len, uint32_t sample_rate);
    bool record(int16_t* recdata, size_t array_len, uint32_t sample_rate);

    void stopPlay(void);
    void stopPlay(uint8_t channel);

    void setVolume(uint8_t master_volume) { _master_volume = master_volume; }
    uint8_t getVolume(void) const { return _master_volume; }

    void setChannelVolume(uint8_t channel, uint8_t volume) { if (channel < sound_channel_max) { _ch_info[channel].volume = volume; } }
    uint8_t getChannelVolume(uint8_t channel) const { return (channel < sound_channel_max) ? _ch_info[channel].volume : 0; }

    size_t isPlaying(void) const { return _play_channel_bits ? 1 : 0; }
    size_t isPlaying(uint8_t channel) const { return _play_channel_bits ? (bool)_ch_info[channel].current_wav.repeat + (bool)_ch_info[channel].next_wav.repeat : 0; }
    size_t isRecording(void) const { return _is_recording ? ((bool)_rec_info[0].length) + ((bool)_rec_info[1].length) : 0; }

    void setCallback(void* args, bool(*func)(void*, sound_mode_t)) { _cb_set_mode = func; _cb_set_mode_args = args; }

  protected:

    struct wav_info_t
    {
      size_t length = 0;
      int repeat = 0;   /// -1 mean infinity repeat
      uint32_t sample_rate = 0;
      const void* data = nullptr;
      bool is_stereo = false;
      bool is_16bit = false;
      bool stop_current = false;
      void clear(void);
    };

    struct channel_info_t
    {
      bool setWav(const wav_info_t& wav);
      wav_info_t current_wav;
      wav_info_t next_wav;
      size_t index = 0;
      int diff = 0;
      uint8_t volume = 128;
    };

    channel_info_t _ch_info[sound_channel_max];

    struct recording_info_t
    {
      void* data = nullptr;
      size_t length = 0;
      bool is_16bit = false;
    };

    recording_info_t _rec_info[2];

    static void output_task(void* args);
    static void input_task(void* args);

    int _calc_rec_rate(void) const;
    bool _set_mode(sound_mode_t mode);
    esp_err_t _setup_i2s(bool mic);
    esp_err_t _write_i2s(int32_t* value, int32_t volume);
    bool _play_raw(const void* wav, size_t array_len, bool flg_16bit, bool flg_stereo, uint32_t sample_rate, uint32_t repeat_count, uint8_t channel);
    bool _rec_raw(void* recdata, size_t array_len, bool flg_16bit, uint32_t sample_rate);
    bool _set_channel(const wav_info_t& info, uint8_t channel);
    uint8_t _autochannel(void);

    sound_mode_t _sound_mode = sound_off;
    i2s_dac_mode_t _dac_mode = i2s_dac_mode_t::I2S_DAC_CHANNEL_DISABLE;

    sound_config_t _cfg;
    size_t _sound_buf_index = 0;
    int16_t _sound_buf[dma_buf_len];

    int32_t _dac_offset = 0;
    uint16_t _surplus[2] = { 0, 0 };

    uint8_t _master_volume = 128;

    bool (*_cb_set_mode)(void* args, sound_mode_t mode) = nullptr;
    void* _cb_set_mode_args = nullptr;

    TaskHandle_t _sound_task_handle = nullptr;
    volatile uint8_t _play_channel_bits = 0;
    volatile bool _task_running = false;
    volatile bool _is_recording = false;
    uint32_t _rec_sample_rate = 0;
  };
}

#endif
