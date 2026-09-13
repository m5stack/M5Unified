// AudioOutputM5Speaker.h
//
// ESP8266Audio output that feeds M5.Speaker. The decoded PCM goes into
// buffers used in rotation; a buffer is refilled only after the speaker task
// has released it, which it reports through setBufferReleaseCallback.
//
// An Arduino sketch can only include files from its own folder, so this file
// is copied into each example that uses it. Keep the copies identical.
#pragma once

#include <AudioOutput.h>
#include <M5Unified.h>
#include <atomic>

class AudioOutputM5Speaker : public AudioOutput
{
  public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0)
    {
      _m5sound = m5sound;
      _virtual_ch = virtual_sound_channel;
    }
    virtual ~AudioOutputM5Speaker(void) {};
    /// Call once after M5.begin() and before anything plays on the speaker
    /// (the callback may only be registered before the first request).
    /// Not done in the constructor: at global construction M5 may not exist.
    void setup(void)
    {
      // Two buffers would be enough for the handshake with the speaker task;
      // the third keeps one buffer of slack against decoder and network
      // jitter.
      _m5sound->setBufferReleaseCallback(this, bufferReleased);
    }
    /// Called with every buffer as it is queued (from the decoding task),
    /// e.g. to feed a display. Must return quickly; the buffer is reused
    /// after the call.
    typedef void (*monitor_fn_t)(void* arg, const int16_t* stereo, size_t frames);
    void setMonitor(monitor_fn_t fn, void* arg) { _monitor = fn; _monitor_arg = arg; }
    virtual bool begin(void) override { return true; }
    virtual bool ConsumeSample(int16_t sample[2]) override
    {
      if (_buffer_index < buf_size)
      {
        _buffer[_index][_buffer_index  ] = sample[0];
        _buffer[_index][_buffer_index+1] = sample[1];
        _buffer_index += 2;

        return true;
      }

      flush();
      return false;
    }
    virtual void flush(void) override
    {
      if (_buffer_index)
      {
        // busy is set before playRaw: the release can only come after the
        // request is queued. playRaw returns false when nothing was queued.
        _busy[_index] = true;
        bool queued = _m5sound->playRaw(_buffer[_index], _buffer_index, hertz, true, 1, _virtual_ch);
        if (queued) { _frames += _buffer_index / 2; }
        else { _busy[_index] = false; }
        // The display gets what the speaker got: nothing when the request was not queued.
        if (_monitor) { _monitor(_monitor_arg, queued ? _buffer[_index] : nullptr, queued ? _buffer_index / 2 : 0); }
        if (++_index >= buf_count) { _index = 0; }
        _buffer_index = 0;
        while (_busy[_index]) { vTaskDelay(1); } // wait until the speaker task has released the next buffer
      }
    }
    virtual bool stop(void) override
    {
      // Let the queued buffers play out rather than cutting them: after the
      // last release nothing refers to the buffers any more.
      flush();
      for (size_t i = 0; i < buf_count; ++i)
      {
        while (_busy[i]) { vTaskDelay(1); }
      }
      if (_monitor) { _monitor(_monitor_arg, nullptr, 0); } // silence from here
      return true;
    }
    /// stereo frames the speaker accepted so far, and their rate: what has
    /// actually been played, unlike wall-clock time spent buffering. The
    /// rate is 0 until the decoder has produced its first sample.
    uint32_t getFrames(void) const { return _frames; }
    uint32_t getRate(void) const { return hertz; }

  protected:
    m5::Speaker_Class* _m5sound;
    uint8_t _virtual_ch;
    static constexpr size_t buf_count = 3;
    // 512 stereo frames: the display shows one buffer, with room to find its
    // trigger edge (AudioUI::raw_frames).
    static constexpr size_t buf_size = 1024;
    int16_t _buffer[buf_count][buf_size];
    // These flags are shared between tasks. Use std::atomic<bool> here;
    // plain bool or volatile does not safely share updates between tasks.
    std::atomic<bool> _busy[buf_count] = { {false}, {false}, {false} };
    size_t _buffer_index = 0;
    size_t _index = 0;
    uint32_t _frames = 0;
    monitor_fn_t _monitor = nullptr;
    void* _monitor_arg = nullptr;

    static void bufferReleased(void* args, const void* data, uint8_t)
    {
      auto me = (AudioOutputM5Speaker*)args;
      // The speaker has finished using this buffer; it can be refilled now.
      for (size_t i = 0; i < buf_count; ++i) { if (data == me->_buffer[i]) { me->_busy[i] = false; } }
    }
};
