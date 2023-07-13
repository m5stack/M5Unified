#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>

static constexpr const size_t record_number = 256;
static constexpr const size_t record_length = 200;
static constexpr const size_t record_size = record_number * record_length;
static constexpr const size_t record_samplerate = 16000;
static int16_t prev_y[record_length];
static int16_t prev_h[record_length];
static size_t rec_record_idx = 2;
static size_t draw_record_idx = 0;
static int16_t *rec_data;

#if !defined ( ARDUINO )
 void delay(uint32_t msec) { vTaskDelay(msec / portTICK_PERIOD_MS ); }
#endif

void setup(void)
{
  auto cfg = M5.config();

// cfg.external_speaker.hat_spk = true;     /// use external speaker (HAT SPK)
// cfg.external_speaker.hat_spk2 = true;    /// use external speaker (HAT SPK2)
// cfg.external_speaker.atomic_spk = true;  /// use external speaker (ATOMIC SPK)

  M5.begin(cfg);

  M5.Display.startWrite();

  if (M5.Display.width() > M5.Display.height())
  {
    M5.Display.setRotation(M5.Display.getRotation()^1);
  }

  M5.Display.setCursor(0, 0);
  M5.Display.print("REC");
  rec_data = (typeof(rec_data))heap_caps_malloc(record_size * sizeof(int16_t), MALLOC_CAP_8BIT);
  memset(rec_data, 0 , record_size * sizeof(int16_t));
  M5.Speaker.setVolume(255);

  /// Since the microphone and speaker cannot be used at the same time, turn off the speaker here.
  M5.Speaker.end();
  M5.Mic.begin();
}

void loop(void)
{
  M5.update();
 
  if (M5.Mic.isEnabled())
  {
    static constexpr int shift = 6;
    auto data = &rec_data[rec_record_idx * record_length];
    if (M5.Mic.record(data, record_length, record_samplerate))
    {
      data = &rec_data[draw_record_idx * record_length];

      int32_t w = M5.Display.width();
      if (w > record_length - 1) { w = record_length - 1; }
      for (int32_t x = 0; x < w; ++x)
      {
        M5.Display.writeFastVLine(x, prev_y[x], prev_h[x], TFT_BLACK);
        int32_t y1 = (data[x    ] >> shift);
        int32_t y2 = (data[x + 1] >> shift);
        if (y1 > y2)
        {
          int32_t tmp = y1;
          y1 = y2;
          y2 = tmp;
        }
        int32_t y = (M5.Display.height() >> 1) + y1;
        int32_t h = (M5.Display.height() >> 1) + y2 + 1 - y;
        prev_y[x] = y;
        prev_h[x] = h;
        M5.Display.writeFastVLine(x, y, h, TFT_WHITE);
      }
      M5.Display.display();

      if (++draw_record_idx >= record_number) { draw_record_idx = 0; }
      if (++rec_record_idx >= record_number) { rec_record_idx = 0; }
    }
  }

  if (M5.BtnA.wasHold() || M5.BtnB.wasClicked())
  {
    auto cfg = M5.Mic.config();
    cfg.noise_filter_level = (cfg.noise_filter_level + 8) & 255;
    M5.Mic.config(cfg);
    M5.Display.setCursor(32,0);
    M5.Display.printf("nf:%03d", cfg.noise_filter_level);
  }
  else
  if (M5.BtnA.wasClicked() || (M5.Touch.getCount() && M5.Touch.getDetail(0).wasClicked()))
  {
    if (M5.Speaker.isEnabled())
    {
      M5.Display.clear();
      while (M5.Mic.isRecording()) { delay(1); }

      /// Since the microphone and speaker cannot be used at the same time, turn off the microphone here.
      M5.Mic.end();
      M5.Speaker.begin();

      M5.Display.setCursor(0,0);
      M5.Display.print("PLAY");
      int start_pos = rec_record_idx * record_length;
      if (start_pos < record_size)
      {
        M5.Speaker.playRaw(&rec_data[start_pos], record_size - start_pos, record_samplerate, false, 1, 0);
      }
      if (start_pos > 0)
      {
        M5.Speaker.playRaw(rec_data, start_pos, record_samplerate, false, 1, 0);
      }
      do
      {
        delay(1);
        M5.update();
      } while (M5.Speaker.isPlaying());

      /// Since the microphone and speaker cannot be used at the same time, turn off the speaker here.
      M5.Speaker.end();
      M5.Mic.begin();

      M5.Display.clear();
      M5.Display.setCursor(0,0);
      M5.Display.print("REC");
    }
  }  
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
