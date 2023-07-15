#include <M5Unified.h>

void setup(void)
{
  M5.begin();

  /// For models with EPD : refresh control
  M5.Display.setEpdMode(epd_mode_t::epd_fastest); // fastest but very-low quality.

  if (M5.Display.width() < M5.Display.height())
  { /// Landscape mode.
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }
}

void loop(void)
{
  M5.delay(1);

  M5.update();
//------------------- Button test
/*
/// List of available buttons:
  M5Stack BASIC/GRAY/GO/FIRE:  BtnA,BtnB,BtnC
  M5Stack Core2:               BtnA,BtnB,BtnC,BtnPWR
  M5Stick C/CPlus:             BtnA,BtnB,     BtnPWR
  M5Stick CoreInk:             BtnA,BtnB,BtnC,BtnPWR,BtnEXT
  M5Paper:                     BtnA,BtnB,BtnC
  M5Station:                   BtnA,BtnB,BtnC,BtnPWR
  M5Tough:                                    BtnPWR
  M5Atom M5AtomU:              BtnA
  M5Stamp Pico/C3/C3U:         BtnA
*/

  static constexpr const int colors[] = { TFT_WHITE, TFT_CYAN, TFT_RED, TFT_YELLOW, TFT_BLUE, TFT_GREEN };
  static constexpr const char* const names[] = { "none", "wasHold", "wasClicked", "wasPressed", "wasReleased", "wasDeciedCount" };

  int w = M5.Display.width() / 5;
  int h = M5.Display.height();
  M5.Display.startWrite();

  /// BtnPWR: "wasClicked"/"wasHold"  can be use.
  /// BtnPWR of CoreInk: "isPressed"/"wasPressed"/"isReleased"/"wasReleased"/"wasClicked"/"wasHold"/"isHolding"  can be use.
  int state = M5.BtnPWR.wasHold() ? 1
            : M5.BtnPWR.wasClicked() ? 2
            : M5.BtnPWR.wasPressed() ? 3
            : M5.BtnPWR.wasReleased() ? 4
            : M5.BtnPWR.wasDecideClickCount() ? 5
            : 0;

  if (state)
  {
    M5_LOGI("BtnPWR:%s  count:%d", names[state], M5.BtnPWR.getClickCount());
    M5.Display.fillRect(w*0, 0, w-1, h, colors[state]);
  }

  /// BtnA,BtnB,BtnC,BtnEXT: "isPressed"/"wasPressed"/"isReleased"/"wasReleased"/"wasClicked"/"wasHold"/"isHolding"  can be use.
  state = M5.BtnA.wasHold() ? 1
        : M5.BtnA.wasClicked() ? 2
        : M5.BtnA.wasPressed() ? 3
        : M5.BtnA.wasReleased() ? 4
        : M5.BtnA.wasDecideClickCount() ? 5
        : 0;
  if (state)
  {
    M5_LOGI("BtnA:%s  count:%d", names[state], M5.BtnA.getClickCount());
    M5.Display.fillRect(w*1, 0, w-1, h, colors[state]);
  }

  state = M5.BtnB.wasHold() ? 1
        : M5.BtnB.wasClicked() ? 2
        : M5.BtnB.wasPressed() ? 3
        : M5.BtnB.wasReleased() ? 4
        : M5.BtnB.wasDecideClickCount() ? 5
        : 0;
  if (state)
  {
    M5_LOGI("BtnB:%s  count:%d", names[state], M5.BtnB.getClickCount());
    M5.Display.fillRect(w*2, 0, w-1, h, colors[state]);
  }

  state = M5.BtnC.wasHold() ? 1
        : M5.BtnC.wasClicked() ? 2
        : M5.BtnC.wasPressed() ? 3
        : M5.BtnC.wasReleased() ? 4
        : M5.BtnC.wasDecideClickCount() ? 5
        : 0;
  if (state)
  {
    M5_LOGI("BtnC:%s  count:%d", names[state], M5.BtnC.getClickCount());
    M5.Display.fillRect(w*3, 0, w-1, h, colors[state]);
  }

  state = M5.BtnEXT.wasHold() ? 1
        : M5.BtnEXT.wasClicked() ? 2
        : M5.BtnEXT.wasPressed() ? 3
        : M5.BtnEXT.wasReleased() ? 4
        : M5.BtnEXT.wasDecideClickCount() ? 5
        : 0;
  if (state)
  {
    M5_LOGI("BtnEXT:%s  count:%d", names[state], M5.BtnEXT.getClickCount());
    M5.Display.fillRect(w*4, 0, w-1, h, colors[state]);
  }
  M5.Display.endWrite();
}

#if !defined ( ARDUINO ) && defined ( ESP_PLATFORM )
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
