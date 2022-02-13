// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_Speaker_Class_H__
#define __M5_Speaker_Class_H__

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2s.h>

namespace m5
{
  class M5Unified;

  struct speaker_config_t
  {
    /// i2s_data_out (for spk)
    int pin_data_out = I2S_PIN_NO_CHANGE;

    /// i2s_bck
    int pin_bck = I2S_PIN_NO_CHANGE;

    /// i2s_ws (lrck)
    int pin_ws = I2S_PIN_NO_CHANGE;

    /// output sampling rate (Hz)
    uint32_t sample_rate = 48000;

    /// use stereo output
    bool stereo = false;

    /// use single gpio buzzer, ( need only pin_data_out )
    bool buzzer = false;

    /// use DAC speaker, ( need only pin_data_out ) ( only GPIO_NUM_25 or GPIO_NUM_26 )
    bool use_dac = false;

    /// multiplier for output value
    uint8_t magnification = 16;

    /// background task priority
    UBaseType_t task_priority = configMAX_PRIORITIES - 3;

    /// I2S port
    i2s_port_t i2s_port = i2s_port_t::I2S_NUM_0;
  };

  class Speaker_Class
  {
  friend M5Unified;

    static constexpr const size_t sound_channel_max = 8;

    static const uint8_t _default_tone_wav[2];

  public:

    speaker_config_t config(void) const { return _cfg; }
    void config(const speaker_config_t& cfg) { _cfg = cfg; }

    bool begin(void);

    void end(void);

    bool isRunning(void) const { return _task_running; }

    bool isEnabled(void) const { return _cfg.pin_data_out >= 0; }

    /// now in playing or not.
    /// @return false=not playing / true=playing
    bool isPlaying(void) const { return _play_channel_bits ? true : false; }

    /// now in playing or not.
    /// @param channel virtual channel number. (0~7), (default = automatically selected)
    /// @return 0=not playing / 1=playing (There's room in the queue) / 2=playing (There's no room in the queue.)
    size_t isPlaying(uint8_t channel) const { return (channel < sound_channel_max) ? (bool)_ch_info[channel].next_wav.repeat + (bool)_ch_info[channel].current_wav.repeat : 0; }

    /// sets the output master volume of the sound.
    /// @param master_volume master volume (0~255)
    void setVolume(uint8_t master_volume) { _master_volume = master_volume; }

    /// gets the output master volume of the sound.
    /// @return master volume.
    uint8_t getVolume(void) const { return _master_volume; }

    /// sets the output volume of the sound for the all virtual channel.
    /// @param volume channel volume (0~255)
    void setAllChannelVolume(uint8_t volume) { for (size_t ch = 0; ch < sound_channel_max; ++ch) { _ch_info[ch].volume = volume; } }

    /// sets the output volume of the sound for the specified virtual channel.
    /// @param channel virtual channel number. (0~7)
    /// @param volume channel volume (0~255)
    void setChannelVolume(uint8_t channel, uint8_t volume) { if (channel < sound_channel_max) { _ch_info[channel].volume = volume; } }

    /// gets the output volume of the sound for the specified virtual channel.
    /// @param channel virtual channel number. (0~7)
    /// @return channel volume.
    uint8_t getChannelVolume(uint8_t channel) const { return (channel < sound_channel_max) ? _ch_info[channel].volume : 0; }

    /// stop sound output.
    void stop(void);

    /// stop sound output for the specified virtual channel.
    /// @param channel virtual channel number. (0~7)
    void stop(uint8_t channel);

    /// play simple tone sound.
    /// @param frequency tone frequency (Hz)
    /// @param duration tone duration (msec)
    /// @param channel virtual channel number. (0~7), (default = automatically selected)
    /// @param stop_current_sound true=start a new output without waiting for the current one to finish.
    /// @param wav_data Single amplitude audio data. 8bit unsigned wav.
    /// @param array_len size of wav_data.
    /// @param stereo true=data is stereo / false=data is mono.
    bool tone(float frequency, uint32_t duration, int channel, bool stop_current_sound, const uint8_t* wav_data, size_t array_len, bool stereo = false);

    /// play simple tone sound.
    /// @param frequency tone frequency (Hz)
    /// @param duration tone duration (msec)
    /// @param channel virtual channel number. (0~7), (default = automatically selected)
    bool tone(float frequency, uint32_t duration = ~0u, int channel = -1, bool stop_current_sound = true) { return tone(frequency, duration, channel, stop_current_sound, _default_tone_wav, sizeof(_default_tone_wav), false); }

    /// play raw sound wave data. (for signed 8bit wav data)
    /// @param wav_data wave data.
    /// @param array_len Number of data array elements.
    /// @param sample_rate the sampling rate (Hz) (default = 44100)
    /// @param stereo true=data is stereo / false=data is monaural.
    /// @param repeat number of times played repeatedly. (default = 1)
    /// @param channel virtual channel number (If omitted, use an available channel.)
    /// @param stop_current_sound true=start a new output without waiting for the current one to finish.
    bool playRAW(const int8_t* wav_data, size_t array_len, uint32_t sample_rate = 44100, bool stereo = false, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false)
    {
      return _play_raw(static_cast<const void* >(wav_data), array_len, false, true, sample_rate, stereo, repeat, channel, stop_current_sound, false);
    }

    /// play raw sound wave data. (for unsigned 8bit wav data)
    /// @param wav_data wave data.
    /// @param array_len Number of data array elements.
    /// @param sample_rate the sampling rate (Hz) (default = 44100)
    /// @param stereo true=data is stereo / false=data is monaural.
    /// @param repeat number of times played repeatedly. (default = 1)
    /// @param channel virtual channel number (If omitted, use an available channel.)
    /// @param stop_current_sound true=start a new output without waiting for the current one to finish.
    bool playRAW(const uint8_t* wav_data, size_t array_len, uint32_t sample_rate = 44100, bool stereo = false, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false)
    {
      return _play_raw(static_cast<const void* >(wav_data), array_len, false, false, sample_rate, stereo, repeat, channel, stop_current_sound, false);
    }

    /// play raw sound wave data. (for signed 16bit wav data)
    /// @param wav_data wave data.
    /// @param array_len Number of data array elements.
    /// @param sample_rate the sampling rate (Hz) (default = 44100)
    /// @param stereo true=data is stereo / false=data is monaural.
    /// @param repeat number of times played repeatedly. (default = 1)
    /// @param channel virtual channel number (If omitted, use an available channel.)
    /// @param stop_current_sound true=start a new output without waiting for the current one to finish.
    bool playRAW(const int16_t* wav_data, size_t array_len, uint32_t sample_rate = 44100, bool stereo = false, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false)
    {
      return _play_raw(static_cast<const void* >(wav_data), array_len, true, true, sample_rate, stereo, repeat, channel, stop_current_sound, false);
    }

  protected:

    void setCallback(void* args, bool(*func)(void*, bool)) { _cb_set_enabled = func; _cb_set_enabled_args = args; }

    struct wav_info_t
    {
      size_t length = 0;
      uint32_t repeat = 0;   /// -1 mean infinity repeat
      uint32_t sample_rate = 0;
      const void* data = nullptr;
      union
      {
        uint8_t flg = 0;
        struct
        {
          uint8_t is_stereo      : 1;
          uint8_t is_16bit       : 1;
          uint8_t is_signed      : 1;
          uint8_t stop_current   : 1;
          uint8_t no_clear_index : 1;
        };
      };
      
      void clear(void);
    };

    struct channel_info_t
    {
      bool setWav(const wav_info_t& wav);
      wav_info_t current_wav;
      wav_info_t next_wav;
      size_t index = 0;
      size_t diff = 0;
      uint8_t volume = 64; // channel volume (not master volume)
    };

    channel_info_t _ch_info[sound_channel_max];

    static void output_task(void* args);

    esp_err_t _setup_i2s(void);
    bool _play_raw(const void* wav, size_t array_len, bool flg_16bit, bool flg_signed, uint32_t sample_rate, bool flg_stereo, uint32_t repeat_count, int channel, bool stop_current_sound, bool no_clear_index);

    speaker_config_t _cfg;
    uint8_t _master_volume = 64;

    bool (*_cb_set_enabled)(void* args, bool enabled) = nullptr;
    void* _cb_set_enabled_args = nullptr;

    TaskHandle_t _task_handle = nullptr;
    volatile bool _task_running = false;
    volatile uint8_t _play_channel_bits = 0;
  };
}

#endif
