
#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>
#include <atomic>

/// need ESP32-A2DP library. ( URL : https://github.com/pschatzmann/ESP32-A2DP/ )
#include <BluetoothA2DPSink.h>

// Shared with the other audio examples (same file in each folder).
#include "AudioUI.h"

// Some cores (ESP32-P4) do not name the second CPU; the speaker task goes there when there is one.
#ifndef APP_CPU_NUM
#define APP_CPU_NUM ((SOC_CPU_CORES_NUM > 1) ? 1 : 0)
#endif

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

/// set ESP32-A2DP device name
static constexpr char bt_device_name[] = "ESP32";


static AudioUI ui;

class BluetoothA2DPSink_M5Speaker : public BluetoothA2DPSink
{
public:
  BluetoothA2DPSink_M5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_channel = 0)
  : BluetoothA2DPSink()
  {
    // The PCM goes to M5.Speaker through audio_data_callback below: no I2S
    // output by the library. (Same call on the old and the current API.)
    set_stream_reader(nullptr, false);
    _display_lock = xSemaphoreCreateMutex();
  }

  void clear(void)
  { // The display shows silence from here until audio starts again (audio
    // that is still on its way in audio_data_callback is not shown either:
    // the flag and the display update change together, under _display_lock).
    // The buffers themselves are left alone: the speaker task may still read one.
    if (_display_lock == nullptr) { return; }
    xSemaphoreTake(_display_lock, portMAX_DELAY);
    _stopped = true;
    ui.feed(nullptr, 0);
    xSemaphoreGive(_display_lock);
  }

  // Two buffers used alternately. A buffer is refilled only after the speaker
  // task has released it, which it reports through setBufferReleaseCallback.
  static void bufferReleased(void* args, const void* data, uint8_t)
  {
    auto me = (BluetoothA2DPSink_M5Speaker*)args;
    // The speaker has finished using this buffer; it can be refilled now.
    for (int i = 0; i < 2; ++i) { if (data == me->_buf[i]) { me->_busy[i] = false; } }
  }

  // Volume set from the phone (AVRCP absolute volume, 0-127) drives the
  // speaker. The library's own software volume is bypassed with the output.
  static void volumeChanged(int volume)
  {
    M5.Speaker.setVolume(volume * 255 / 127);
  }


protected:
  // Fixed storage (no reallocation): the addresses the release callback
  // compares against never change. Half of one A2DP packet fits.
  static constexpr size_t buf_samples = 2048;
  int16_t _buf[2][buf_samples];
  // These flags are shared between tasks. Use std::atomic<bool> here;
  // plain bool or volatile does not safely share updates between tasks.
  std::atomic<bool> _busy[2] = { {false}, {false} };
  bool _stopped = true;                 // set by clear() (stop, suspend, disconnect), cleared when audio starts; under _display_lock
  SemaphoreHandle_t _display_lock = nullptr; // orders "stopped + silence" against "running + audio" for the display
  size_t _index = 0;
  size_t _sample_rate = 48000;

  void av_hdl_a2d_evt(uint16_t event, void *p_param) override
  {
    esp_a2d_cb_param_t* a2d = (esp_a2d_cb_param_t *)(p_param);

    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
      if (ESP_A2D_CONNECTION_STATE_CONNECTED == a2d->conn_stat.state)
      { // 接続

      }
      else
      if (ESP_A2D_CONNECTION_STATE_DISCONNECTED == a2d->conn_stat.state)
      { // 切断: the event itself clears the display (the connection poll in loop can miss a short connection)
        ui.clearMeta();
        clear();
      }
      break;

    case ESP_A2D_AUDIO_STATE_EVT:
      if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state)
      { // 再生
        if (_display_lock) { xSemaphoreTake(_display_lock, portMAX_DELAY); _stopped = false; xSemaphoreGive(_display_lock); }
      } else
      if ( ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND == a2d->audio_stat.state
        || ESP_A2D_AUDIO_STATE_STOPPED        == a2d->audio_stat.state )
      { // 停止
        ui.clearMeta(); // nothing from the previous track stays on screen
        clear();
      }
      break;

    case ESP_A2D_AUDIO_CFG_EVT:
      {
        esp_a2d_cb_param_t *a2d = (esp_a2d_cb_param_t *)(p_param);
        size_t tmp = a2d->audio_cfg.mcc.cie.sbc[0];
        size_t rate = 16000;
        if (     tmp & (1 << 6)) { rate = 32000; }
        else if (tmp & (1 << 5)) { rate = 44100; }
        else if (tmp & (1 << 4)) { rate = 48000; }
        _sample_rate = rate;
      }
      break;

    default:
      break;
    }

    BluetoothA2DPSink::av_hdl_a2d_evt(event, p_param);
  }

  void av_hdl_avrc_evt(uint16_t event, void *p_param) override
  {
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);

    switch (event)
    {
    case ESP_AVRC_CT_METADATA_RSP_EVT:
      { // title / artist / album (ESP_AVRC_MD_ATTR_TITLE, _ARTIST, _ALBUM) on the header lines
        static constexpr const char* labels[] = { "Title", "Artist", "Album" };
        for (size_t i = 0; i < 3; ++i)
        {
          if (0 == (rc->meta_rsp.attr_id & (1 << i))) { continue; }
          ui.setMeta(i, (const char*)rc->meta_rsp.attr_text, labels[i]);
          break;
        }
      }
      break;

    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
      break;

    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
      if (rc->change_ntf.event_id == ESP_AVRC_RN_TRACK_CHANGE)
      {
        ui.clearMeta(); // the new track's attributes arrive one by one: nothing of the old one stays
      }
      break;

    default:
      break;
    }

    BluetoothA2DPSink::av_hdl_avrc_evt(event, p_param);
  }

  int16_t* get_next_buf(const uint8_t* src_data, uint32_t& len)
  {
    size_t idx = _index ^ 1;
    while (_busy[idx]) { vTaskDelay(1); } // wait until the speaker task has released this buffer
    if (len > sizeof(_buf[idx])) { len = sizeof(_buf[idx]); } // a longer packet than expected: keep what fits
    memcpy(_buf[idx], src_data, len);
    _index = idx;
    return _buf[idx];
  }

  /// true when the buffer was queued on the speaker
  bool play_buf(const int16_t* buf, uint32_t len)
  {
    if (len > sizeof(_buf[0])) { len = sizeof(_buf[0]); } // never queue more than the storage holds
    if (len < 2) { return false; }
    // busy is set before playRaw: the release can only come after the
    // request is queued. playRaw returns false when nothing was queued.
    _busy[_index] = true;
    bool queued = M5.Speaker.playRaw(buf, len >> 1, _sample_rate, true, 1, m5spk_virtual_channel);
    if (!queued) { _busy[_index] = false; }
    return queued;
  }

  void audio_data_callback(const uint8_t *data, uint32_t length) override
  {
    // Reduce memory requirements by dividing the received data into the first and second halves.
    length >>= 1;
    // separate statements: get_next_buf may shorten len, and the order in
    // which call arguments are evaluated is unspecified.
    uint32_t len = length;
    auto buf = get_next_buf(data, len);
    play_buf(buf, len);
    len = length;
    buf = get_next_buf(&data[length], len);
    bool queued = play_buf(buf, len);
    // The display gets what the speaker got (bytes to stereo frames; it takes
    // its own copy), and nothing after a stop: the flag is checked and the
    // display fed under the same lock that clear() uses, so a packet still in
    // flight here cannot overwrite the stop's silence.
    if (_display_lock)
    {
      xSemaphoreTake(_display_lock, portMAX_DELAY);
      if (!_stopped) { ui.feed(queued ? buf : nullptr, queued ? (len >> 2) : 0); }
      xSemaphoreGive(_display_lock);
    }
  }
};
static BluetoothA2DPSink_M5Speaker a2dp_sink = { &M5.Speaker, m5spk_virtual_channel };
void setup(void)
{
  auto cfg = M5.config();

  // If you want to play sound from ModuleDisplay, write this
//  cfg.external_speaker.module_display = true;

  // If you want to play sound from ModuleRCA, write this
//  cfg.external_speaker.module_rca     = true;

  // If you want to play sound from HAT Speaker, write this
  cfg.external_speaker.hat_spk        = true;

  // If you want to play sound from HAT Speaker2, write this
//  cfg.external_speaker.hat_spk2       = true;

  // If you want to play sound from ATOMIC Speaker, write this
  cfg.external_speaker.atomic_spk     = true;

  M5.begin(cfg);
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO); // track information goes to the serial log too

  M5.Speaker.setBufferReleaseCallback(&a2dp_sink, BluetoothA2DPSink_M5Speaker::bufferReleased);
  a2dp_sink.set_on_volumechange(BluetoothA2DPSink_M5Speaker::volumeChanged);

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    // spk_cfg.task_priority = configMAX_PRIORITIES - 2;
    spk_cfg.dma_buf_count = 20;
    // spk_cfg.dma_buf_len = 512;
    M5.Speaker.config(spk_cfg);
  }


  M5.Speaker.begin();

  ui.setup(&M5.Display, (String("BT A2DP : ") + bt_device_name).c_str());

  a2dp_sink.start(bt_device_name, false);
}

void loop(void)
{
  // A disconnect clears the display from the event itself (av_hdl_a2d_evt):
  // polling is_connected() here could clear what a quick reconnect already set.
  ui.loop();

  {
    static int prev_frame;
    int frame;
    do
    {
      vTaskDelay(1);
    } while (prev_frame == (frame = millis() >> 3)); /// 8 msec cycle wait
    prev_frame = frame;
  }

  M5.update();
  auto in = ui.update();
  if (in.track > 0) { a2dp_sink.next(); }
  else if (in.track < 0) { a2dp_sink.previous(); }
  if (in.volume) { a2dp_sink.set_volume(M5.Speaker.getVolume() * 127 / 255); } // keep the phone's volume display in step
}

#if !defined ( ARDUINO )
extern "C" {
  void loopTask(void*)
  {
    setup();
    for (;;) {
      loop();
    }
    vTaskDelete(NULL);
  }

  void app_main()
  {
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
  }
}
#endif
