#include <M5Unified.h>

#include <esp_log.h>

void setup(void)
{
  M5.begin();

// M5.Power.setExtPower(true);  // If you need external port 5V output.
// M5.Imu.begin();              // If you need IMU
// SD.begin(4, SPI, 25000000);  // If you need SD card access
// Serial.begin(115200);        // If you need Serial

  M5.Display.setEpdMode(epd_mode_t::epd_fastest); /// For models with EPD: refresh control

  M5.Display.setBrightness(160); /// For models with LCD: backlight control (0~255)

  if (M5.Display.width() < M5.Display.height())
  { /// Landscape mode.
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }
}

void loop(void)
{
  vTaskDelay(1);

  M5.update();

/*
  /// List of available buttons:
  M5Stack BASIC/GRAY/GO/FIRE:  BtnA,BtnB,BtnC
  M5Stack Core2:               BtnA,BtnB,BtnC,BtnPWR
  M5Stick C/CPlus:             BtnA,BtnB,     BtnPWR
  M5Stick CoreInk:             BtnA,BtnB,BtnC,BtnPWR,BtnEXT
  M5Paper:                     BtnA,BtnB,BtnC
  M5Station:                   BtnA,BtnB,BtnC
  M5Tough:                                    BtnPWR
*/

  M5.Display.startWrite();

  /// BtnPWR: can "wasClicked"/"wasHold"  can be use.
  int state = M5.BtnPWR.wasHold() ? 1
            : M5.BtnPWR.wasClicked() ? 2
            : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnPWR:%s", (state == 1) ? "wasHold" : "wasClicked");
    M5.Display.fillRect(0, 0, 38, 38, (state == 1) ? TFT_CYAN : TFT_RED);
  }

  /// BtnA,BtnB,BtnC,BtnEXT, BtnPWR of CoreInk: "isPressed"/"wasPressed"/"isReleased"/"wasReleased"/"wasClicked"/"wasHold"/"isHolding"  can be use.
  state = M5.BtnA.wasPressed() ? 1
        : M5.BtnA.wasReleased() ? 2
        : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnA:%s", (state == 1) ? "wasPressed" : "wasRelease");
    M5.Display.fillRect(40, 0, 38, 38, (state == 1) ? TFT_YELLOW : TFT_BLUE);
  }

  state = M5.BtnB.wasPressed() ? 1
        : M5.BtnB.wasReleased() ? 2
        : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnB:%s", (state == 1) ? "wasPressed" : "wasRelease");
    M5.Display.fillRect(80, 0, 38, 38, (state == 1) ? TFT_YELLOW : TFT_BLUE);
  }

  state = M5.BtnC.wasPressed() ? 1
        : M5.BtnC.wasReleased() ? 2
        : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnC:%s", (state == 1) ? "wasPressed" : "wasRelease");
    M5.Display.fillRect(120, 0, 38, 38, (state == 1) ? TFT_YELLOW : TFT_BLUE);
  }

  state = M5.BtnEXT.wasPressed() ? 1
        : M5.BtnEXT.wasReleased() ? 2
        : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnEXT:%s", (state == 1) ? "wasPressed" : "wasRelease");
    M5.Display.fillRect(160, 0, 38, 38, (state == 1) ? TFT_YELLOW : TFT_BLUE);
  }

  M5.Display.endWrite();
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
