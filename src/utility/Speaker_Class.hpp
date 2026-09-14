// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_Speaker_Class_H__
#define __M5_Speaker_Class_H__

#include "m5unified_common.h"

#if defined ( ESP_PLATFORM )

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#if __has_include(<driver/i2s_std.h>)
 #include <driver/i2s_std.h>
#else
 #include <driver/i2s.h>
#endif

#endif

#include <atomic>

#ifndef I2S_PIN_NO_CHANGE
#define I2S_PIN_NO_CHANGE (-1)
#endif

namespace m5
{
  class M5Unified;

  struct speaker_config_t
  {
    /// i2s_data_out (for spk)
    int pin_data_out = I2S_PIN_NO_CHANGE;

    /// i2s_bck
    int pin_bck = I2S_PIN_NO_CHANGE;

    /// i2s_mclk
    int pin_mck = I2S_PIN_NO_CHANGE;

    /// i2s_ws (lrck)
    int pin_ws = I2S_PIN_NO_CHANGE;

    /// output sampling rate (Hz)
    uint32_t sample_rate = 48000;

    /// use stereo output
    bool stereo = false;

    /// use single gpio buzzer, ( need only pin_data_out )
    bool buzzer = false;

    /// use DAC speaker, ( need only pin_data_out ) ( for ESP32, only GPIO_NUM_25 or GPIO_NUM_26 )
    /// ※ for ESP32, need `i2s_port = I2S_NUM_0`. ( DAC+I2S_NUM_1 is not available )
    bool use_dac = false;

    /// Zero level reference value when using DAC ( 0=Dynamic change )
    uint8_t dac_zero_level = 0;

    /// multiplier for output value
    uint8_t magnification = 16;

    /// for I2S dma_buf_len (max 1024)
    size_t dma_buf_len = 256;

    /// for I2S dma_buf_count
    size_t dma_buf_count = 8;

    /// background task priority
    uint8_t task_priority = 2;

    /// background task pinned core
    uint8_t task_pinned_core = ~0;

    /// I2S port
    i2s_port_t i2s_port = (i2s_port_t)I2S_NUM_0;
  };

  class Speaker_Class
  {
  friend M5Unified;
  public:
    virtual ~Speaker_Class(void) {}

    speaker_config_t config(void) const { return _cfg; }
    void config(const speaker_config_t& cfg) { _cfg = cfg; }

    bool begin(void);

    void end(void);

    bool isRunning(void) const { return _task_running; }

    bool isEnabled(void) const
    {
#if defined (ESP_PLATFORM)
      return _cfg.pin_data_out >= 0;
#else
      return true;
#endif
    }

    /// now in playing or not.
    /// @return false=not playing / true=playing
    bool isPlaying(void) const volatile { return _play_channel_bits.load(); }

    /// now in playing or not.
    /// @param channel virtual channel number. (0~7), (default = automatically selected)
    /// @return 0=not playing / 1=playing (There's room in the queue) / 2=playing (There's no room in the queue.)
    size_t isPlaying(uint8_t channel) const volatile { return (channel < sound_channel_max) ? _slot_occupied(_ch_info[channel].wavinfo[0]) + _slot_occupied(_ch_info[channel].wavinfo[1]) : 0; }

    /// Get the number of channels that are playing.
    /// @return number of channels that are playing.
    size_t getPlayingChannels(void) const volatile { return __builtin_popcount(_play_channel_bits.load()); }

    /// Register a function called when the playback task has finished reading
    /// the buffer of a request: from then on the caller may overwrite or free
    /// the memory that request used. The notice is per request, not per
    /// memory region: if another request (a queued playRaw() of the same
    /// data, a re-triggered tone) refers to the same memory, it is still in
    /// use until that request is released too. Two buffers used alternately
    /// are enough when the next one is filled from this point (three are
    /// needed without it).
    /// @param args passed through as the first argument.
    /// @param func (args, data, channel): data is the pointer the request was
    ///             read from - what was given to playRaw()/tone(), and for
    ///             playWav() the PCM chunk inside the wav, not the wav pointer;
    ///             channel is the virtual channel it was played on.
    /// @attention Called from the playback task, not an ISR: keep it short,
    ///            never block in it, never call begin()/end() from it. The
    ///            sound itself may still be in the I2S queue - this is buffer
    ///            release, not end of playback.
    /// @attention Requests discarded without being played do not call back:
    ///            a queued one replaced by stop() or stop_current_sound, and
    ///            everything dropped by end(). The one being played when
    ///            stop() cuts it does. Delivery follows the slot release, so
    ///            it can arrive after isPlaying() has already dropped: track
    ///            buffers by pointer, not by counting.
    /// @attention play*() may be called from within: it never waits there and
    ///            returns false if no queue slot is free.
    /// @attention Set or clear it only before the first play*() or after
    ///            end() has returned - not merely while the queue looks
    ///            empty: the task may still be about to call the previous
    ///            function, and the function and args are not swapped as one
    ///            unit.
    void setBufferReleaseCallback(void* args, void (*func)(void* args, const void* data, uint8_t channel)) { _cb_buffer_release_args = args; _cb_buffer_release = func; }

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
    /// @param raw_data Single amplitude audio data. 8bit unsigned wav.
    /// @param array_len size of raw_data.
    /// @param stereo true=data is stereo / false=data is mono.
    bool tone(float frequency, uint32_t duration, int channel, bool stop_current_sound, const uint8_t* raw_data, size_t array_len, bool stereo = false)
    {
      return _play_raw(raw_data, array_len, false, false, frequency * (array_len >> stereo), stereo, (duration != UINT32_MAX) ? (uint32_t)(duration * frequency / 1000) : UINT32_MAX, channel, stop_current_sound, true);
    }

    /// play simple tone sound.
    /// @param frequency tone frequency (Hz)
    /// @param duration tone duration (msec)
    /// @param channel virtual channel number. (0~7), (default = automatically selected)
    bool tone(float frequency, uint32_t duration = UINT32_MAX, int channel = -1, bool stop_current_sound = true) { return tone(frequency, duration, channel, stop_current_sound, _default_tone_wav, sizeof(_default_tone_wav), false); }

    /// play raw sound wave data. (for signed 8bit wav data)
    /// @param raw_data wave data.
    /// @param array_len Number of data array elements.
    /// @param sample_rate the sampling rate (Hz) (default = 44100)
    /// @param stereo true=data is stereo / false=data is monaural.
    /// @param repeat number of times played repeatedly. (default = 1)
    /// @param channel virtual channel number (If omitted, use an available channel.)
    /// @param stop_current_sound true=start a new output without waiting for the current one to finish.
    /// @attention For data generated at runtime, two buffers used alternately are enough if the next one is filled only after the previous one is released, as reported by setBufferReleaseCallback(). Without it use three buffers in sequence (or, with a single producer and no stop() or stop_current_sound in between, wait for isPlaying(channel) to drop below 2 before refilling).
    /// @return true if the request was queued; false if not (no free channel, the speaker could not be started, nothing to play). A queued request is reported by setBufferReleaseCallback() unless it is discarded first (see there).
    /// @attention If noise is present in the output sounds, consider increasing the priority of the task that generates the data.
    bool playRaw(const int8_t* raw_data, size_t array_len, uint32_t sample_rate = 44100, bool stereo = false, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false)
    {
      return _play_raw(static_cast<const void* >(raw_data), array_len, false, true, sample_rate, stereo, repeat, channel, stop_current_sound, false);
    }
    [[deprecated("The playRAW function has been renamed to playRaw")]]
    bool playRAW(const int8_t* raw_data, size_t array_len, uint32_t sample_rate = 44100, bool stereo = false, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false)
    {
      return _play_raw(static_cast<const void* >(raw_data), array_len, false, true, sample_rate, stereo, repeat, channel, stop_current_sound, false);
    }

    /// play raw sound wave data. (for unsigned 8bit wav data)
    /// @param raw_data wave data.
    /// @param array_len Number of data array elements.
    /// @param sample_rate the sampling rate (Hz) (default = 44100)
    /// @param stereo true=data is stereo / false=data is monaural.
    /// @param repeat number of times played repeatedly. (default = 1)
    /// @param channel virtual channel number (If omitted, use an available channel.)
    /// @param stop_current_sound true=start a new output without waiting for the current one to finish.
    /// @attention For data generated at runtime, two buffers used alternately are enough if the next one is filled only after the previous one is released, as reported by setBufferReleaseCallback(). Without it use three buffers in sequence (or, with a single producer and no stop() or stop_current_sound in between, wait for isPlaying(channel) to drop below 2 before refilling).
    /// @return true if the request was queued; false if not (no free channel, the speaker could not be started, nothing to play). A queued request is reported by setBufferReleaseCallback() unless it is discarded first (see there).
    /// @attention If noise is present in the output sounds, consider increasing the priority of the task that generates the data.
    bool playRaw(const uint8_t* raw_data, size_t array_len, uint32_t sample_rate = 44100, bool stereo = false, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false)
    {
      return _play_raw(static_cast<const void* >(raw_data), array_len, false, false, sample_rate, stereo, repeat, channel, stop_current_sound, false);
    }
    [[deprecated("The playRAW function has been renamed to playRaw")]]
    bool playRAW(const uint8_t* raw_data, size_t array_len, uint32_t sample_rate = 44100, bool stereo = false, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false)
    {
      return _play_raw(static_cast<const void* >(raw_data), array_len, false, false, sample_rate, stereo, repeat, channel, stop_current_sound, false);
    }

    /// play raw sound wave data. (for signed 16bit wav data)
    /// @param raw_data wave data.
    /// @param array_len Number of data array elements.
    /// @param sample_rate the sampling rate (Hz) (default = 44100)
    /// @param stereo true=data is stereo / false=data is monaural.
    /// @param repeat number of times played repeatedly. (default = 1)
    /// @param channel virtual channel number (If omitted, use an available channel.)
    /// @param stop_current_sound true=start a new output without waiting for the current one to finish.
    /// @attention For data generated at runtime, two buffers used alternately are enough if the next one is filled only after the previous one is released, as reported by setBufferReleaseCallback(). Without it use three buffers in sequence (or, with a single producer and no stop() or stop_current_sound in between, wait for isPlaying(channel) to drop below 2 before refilling).
    /// @return true if the request was queued; false if not (no free channel, the speaker could not be started, nothing to play). A queued request is reported by setBufferReleaseCallback() unless it is discarded first (see there).
    /// @attention If noise is present in the output sounds, consider increasing the priority of the task that generates the data.
    bool playRaw(const int16_t* raw_data, size_t array_len, uint32_t sample_rate = 44100, bool stereo = false, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false)
    {
      return _play_raw(static_cast<const void* >(raw_data), array_len, true, true, sample_rate, stereo, repeat, channel, stop_current_sound, false);
    }

    /// @deprecated "playRAW" function has been renamed to "playRaw"
    [[deprecated("The playRAW function has been renamed to playRaw")]]
    bool playRAW(const int16_t* raw_data, size_t array_len, uint32_t sample_rate = 44100, bool stereo = false, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false)
    {
      return _play_raw(static_cast<const void* >(raw_data), array_len, true, true, sample_rate, stereo, repeat, channel, stop_current_sound, false);
    }

    /// play WAV format sound data.
    /// @param wav_data wave data. (WAV header included)
    /// @param repeat number of times played repeatedly. (default = 1)
    /// @param channel virtual channel number (If omitted, use an available channel.)
    /// @param stop_current_sound true=start a new output without waiting for the current one to finish.
    bool playWav(const uint8_t* wav_data, size_t data_len = ~0u, uint32_t repeat = 1, int channel = -1, bool stop_current_sound = false);

  protected:

    static constexpr const size_t sound_channel_max = 8;

    static const uint8_t _default_tone_wav[16];

    void setCallback(void* args, bool(*func)(void*, bool)) { _cb_set_enabled = func; _cb_set_enabled_args = args; }

    struct wav_info_t
    {
      uint32_t repeat = 0;   /// -1 mean infinity repeat
      uint32_t sample_rate_x256 = 0;
      const void* data = nullptr;
      size_t length = 0;
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

    // A slot moves empty -> writing -> published (owned by a writer), then
    // published -> playing -> empty (owned by the task). Both claims are a CAS
    // on the state byte, so no one ever reads or writes the payload of a slot
    // someone else holds. The flags of the request ride along in the same byte
    // for the decisions that must not look at the payload: cutting the current
    // sound, refusing to queue behind an endless one, counting isPlaying().
    static constexpr uint8_t wav_phase_mask         = 0x03;
    static constexpr uint8_t wav_phase_empty        = 0x00;
    static constexpr uint8_t wav_phase_writing      = 0x01;
    static constexpr uint8_t wav_phase_published    = 0x02;
    static constexpr uint8_t wav_phase_playing      = 0x03;
    static constexpr uint8_t wav_state_stop_current = 0x04;
    static constexpr uint8_t wav_state_infinite     = 0x08;
    static constexpr uint8_t wav_state_stop_marker  = 0x10;

    struct wav_slot_t
    {
      wav_info_t info;
      std::atomic<uint8_t> state { 0 };
    };

    static bool _slot_occupied(const volatile wav_slot_t& slot)
    {
      // acquire: a producer that sees the slot empty may overwrite the buffer
      // the task has finished reading, so that read must be ordered before.
      uint8_t s = slot.state.load(std::memory_order_acquire);
      return ((s & wav_phase_mask) != wav_phase_empty) && !(s & wav_state_stop_marker);
    }

    struct channel_info_t
    {
      wav_slot_t wavinfo[2]; // request queue slots.
      wav_info_t current;    // the request being played; only the task touches it.
      size_t index = 0;
      int diff = 0;
      volatile uint8_t volume = 255; // channel volume (not master volume)
      /// Which slot the next request goes into. The task moves it as it adopts
      /// a request; a writer reloads it whenever its claim fails.
      std::atomic<bool> flip { false };

      float liner_buf[2][2] = { { 0, 0 }, { 0, 0 } };
    };

    channel_info_t _ch_info[sound_channel_max];

    static void spk_task(void* args);

    esp_err_t _setup_i2s(void);
    bool _in_task(void) const;
    void _end_locked(void);
    bool _play_raw(const void* wav, size_t array_len, bool flg_16bit, bool flg_signed, float sample_rate, bool flg_stereo, uint32_t repeat_count, int channel, bool stop_current_sound, bool no_clear_index);
    bool _set_next_wav(size_t ch, const wav_info_t& wav);

    speaker_config_t _cfg;
    volatile uint8_t _master_volume = 64;

    bool (*_cb_set_enabled)(void* args, bool enabled) = nullptr;
    void* _cb_set_enabled_args = nullptr;
    void (*_cb_buffer_release)(void* args, const void* data, uint8_t channel) = nullptr;
    void* _cb_buffer_release_args = nullptr;

    volatile bool _task_running = false;
    std::atomic<uint16_t> _play_channel_bits = { 0 };
    /// begin() runs from whichever task touches the speaker first, and setup
    /// starts by tearing the port down - so only one call may go through.
    std::atomic<bool> _begin_lock { false };
    /// True only once begin() has fully finished; the lock-free early return
    /// keys on this, so a caller can never see a half-built port as ready.
    std::atomic<bool> _begun { false };
#if defined (SDL_h_)
    std::atomic<SDL_Thread*> _task_handle { nullptr };
#else
    /// atomic: _in_task() reads it from any caller of play*(), possibly
    /// while begin() on another task is still creating the task.
    std::atomic<TaskHandle_t> _task_handle { nullptr };
    volatile SemaphoreHandle_t _task_semaphore = nullptr;
#endif
  };
}

#endif
