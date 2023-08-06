#include <M5Unified.h>

void setup(void)
{
  /// You may output logs to standard output.
  M5_LOGE("this is error LOG");
  M5_LOGW("this is warning LOG");
  M5_LOGI("this is info LOG");
  M5_LOGD("this is debug LOG");
  M5_LOGV("this is verbose LOG");

  M5.begin();
  M5.Speaker.tone(2000, 100, 0, false);
  M5.Speaker.tone(1000, 100, 0, false);

  M5.Display.printf("Please push cursor keys.\nButtonA == Left key\nButtonB == Down key\nButtonC == Right key\nButtonPWR == Up key\n");
}

void loop(void)
{
  M5.delay(8);
  M5.update();
  auto td = M5.Touch.getDetail();
  if (td.isPressed()) {
    M5.Display.fillCircle(td.x, td.y, 64, rand());
    int32_t tone = 880 + td.x + td.y;
    if (tone > 0) {
      M5.Speaker.tone(tone, 50, 0);
    }
  }

  if (M5.BtnPWR.wasClicked()) {
    M5.Speaker.tone(4000, 200);
    M5_LOGD("BtnPWR Clicked");
  }
  if (M5.BtnA.wasClicked()) {
    M5.Speaker.tone(2000, 200);
    M5_LOGI("BtnA Clicked");
  }
  if (M5.BtnB.wasClicked()) {
    M5.Speaker.tone(1000, 200);
    M5_LOGW("BtnB Clicked");
  }
  if (M5.BtnC.wasClicked()) {
    M5.Speaker.tone(500, 200);
    M5_LOGE("BtnC Clicked");
  }
}

#if defined ( ESP_PLATFORM ) && !defined ( ARDUINO )
extern "C" {
int app_main(int, char**)
{
    setup();
    for (;;) {
      loop();
    }
    return 0;
}
}
#endif
