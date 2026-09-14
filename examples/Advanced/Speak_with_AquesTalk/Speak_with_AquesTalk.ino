
#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>
#include <atomic>

/// need AquesTalk library. ( URL : https://www.a-quest.com/ )
#include <aquestalk.h>

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

static constexpr uint8_t LEN_FRAME = 32;

static uint32_t workbuf[AQ_SIZE_WORKBUF];
static TaskHandle_t task_handle = nullptr;
volatile bool is_talking = false;

/// Two buffers used alternately. A buffer is refilled only after the speaker
/// task has released it, which it reports through setBufferReleaseCallback.
static int16_t wav[2][LEN_FRAME];
// These flags are shared between tasks. Use std::atomic<bool> here;
// plain bool or volatile does not safely share updates between tasks.
static std::atomic<bool> wav_busy[2] = { {false}, {false} };

static void wav_released(void*, const void* data, uint8_t)
{ // The speaker has finished using this buffer; it can be refilled now.
  for (int i = 0; i < 2; ++i) { if (data == wav[i]) { wav_busy[i] = false; } }
}

static void talk_task(void*)
{
  int idx = 0;
  for (;;)
  {
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY ); // wait notify
    while (is_talking)
    {
      while (wav_busy[idx]) { vTaskDelay(1); } // wait until the speaker task has released this buffer
      uint16_t len;
      if (CAqTkPicoF_SyntheFrame(wav[idx], &len)) { is_talking = false; break; }
      if (len == 0) { continue; }
      // busy is set before playRaw: the release can only come after the
      // request is queued. playRaw returns false when nothing was queued.
      wav_busy[idx] = true;
      if (!M5.Speaker.playRaw(wav[idx], len, 8000, false, 1, m5spk_virtual_channel, false)) { wav_busy[idx] = false; }
      idx ^= 1;
    }
  }
}

/// 音声再生の終了を待機する;
static void waitAquesTalk(void)
{
  while (is_talking) { vTaskDelay(1); }
}

/// 音声再生を停止する;
static void stopAquesTalk(void)
{
  if (is_talking) { is_talking = false; vTaskDelay(1); }
}

/// 音声再生を開始する。(再生中の場合は中断して新たな音声再生を開始する) ;
static void playAquesTalk(const char *koe)
{
  stopAquesTalk();

  M5.Display.printf("Play:%s\n", koe);

  int iret = CAqTkPicoF_SetKoe((const uint8_t*)koe, 100, 0xFFu);
  if (iret) { M5.Display.println("ERR:CAqTkPicoF_SetKoe"); }

  is_talking = true;
  xTaskNotifyGive(task_handle);
}

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

  M5.Speaker.setBufferReleaseCallback(nullptr, wav_released);
  xTaskCreateUniversal(talk_task, "talk_task", 4096, nullptr, 1, &task_handle, APP_CPU_NUM);
/*
  /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
  auto spk_cfg = M5.Speaker.config();
  spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
  M5.Speaker.config(spk_cfg);
//*/
  M5.Speaker.setVolume(128);

  M5.Display.setEpdMode(epd_mode_t::epd_fastest);
  M5.Display.setTextWrap(true);

  int iret = CAqTkPicoF_Init(workbuf, LEN_FRAME, "XXX-XXX-XXX");
  if (iret) {
    M5.Display.println("ERR:CAqTkPicoF_Init");
  }

  playAquesTalk("akue_suto'-_ku/kido-shima'_shita.");
  waitAquesTalk();
  playAquesTalk("botanno/o_shitekudasa'i.");
}

void loop(void)
{
  M5.update();

  if (     M5.BtnA.wasClicked())  { playAquesTalk("kuri'kku"); }
  else if (M5.BtnA.wasHold())     { playAquesTalk("ho'-rudo"); }
  else if (M5.BtnA.wasReleased()) { playAquesTalk("riri'-su"); }
  else if (M5.BtnB.wasReleased()) { playAquesTalk("korewa;te'_sutode_su."); }
  else if (M5.BtnC.wasReleased()) { playAquesTalk("yukkuri_siteittene?"); }
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
    xTaskCreateUniversal(loopTask, "loopTask", 8192, NULL, 1, NULL, APP_CPU_NUM);
  }
}
#endif
