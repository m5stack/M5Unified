
#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>

/// need AquesTalk library. ( URL : https://www.a-quest.com/ )
#include <aquestalk.h>

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

static constexpr uint8_t LEN_FRAME = 32;

static uint32_t workbuf[AQ_SIZE_WORKBUF];
static int16_t wav[3][LEN_FRAME];
static int tri_index = 0;

void playAquesTalk(const char *koe)
{
  M5.Display.printf("Play:%s\n", koe);

  int iret = CAqTkPicoF_SetKoe((const uint8_t*)koe, 100, 0xFFu);
  if (iret) { M5.Display.println("ERR:CAqTkPicoF_SetKoe"); }

  for (;;)
  {
    uint16_t len;
    iret = CAqTkPicoF_SyntheFrame(wav[tri_index], &len);
    if (iret) { return; }

    while (!M5.Speaker.playRAW(wav[tri_index], len, 8000, false, 1, m5spk_virtual_channel, false)) { taskYIELD(); }
    tri_index = tri_index < 2 ? tri_index + 1 : 0;
  }
}

void setup(void)
{
  auto cfg = M5.config();

//cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
//cfg.external_spk_detail.omit_atomic_spk = true; // exclude ATOMIC SPK
//cfg.external_spk_detail.omit_spk_hat    = true; // exclude SPK HAT

  M5.begin(cfg);

/*
  /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
  auto spk_cfg = M5.Speaker.config();
  spk_cfg.sample_rate = 125000; // default:48000 (48kHz)
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
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
  }
}
#endif
