#include <M5Unified.h>

static void test_display(void)
{
  M5.Display.setBrightness(128);
  M5.Display.setColorDepth(16);
  M5.Display.setRotation(1);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.drawString("M5Unified", 0, 0);
  M5.Display.printf("board=%d\n", (int)M5.getBoard());

  for (size_t i = 0; i < M5.getDisplayCount(); ++i) {
    M5.Displays(i).startWrite();
    M5.Displays(i).drawPixel(0, 0, 0xFFFF);
    M5.Displays(i).endWrite();
  }
}

static void test_speaker(void)
{
  if (!M5.Speaker.isEnabled()) { return; }
  M5.Speaker.setVolume(64);
  M5.Speaker.tone(1000, 50);
  M5.Speaker.tone(2000, 50, 0, false);
  M5.Speaker.stop();
}

static void test_mic(void)
{
  if (!M5.Mic.isEnabled()) { return; }
  static int16_t buf[64];
  M5.Mic.record(buf, sizeof(buf) / sizeof(buf[0]), 16000);
  M5.Mic.end();
}

static void test_imu(void)
{
  if (!M5.Imu.isEnabled()) { return; }
  float ax, ay, az;
  float gx, gy, gz;
  float mx, my, mz;
  M5.Imu.getAccel(&ax, &ay, &az);
  M5.Imu.getGyro(&gx, &gy, &gz);
  M5.Imu.getMag(&mx, &my, &mz);
  (void)M5.Imu.getType();
}

static void test_touch(void)
{
  if (!M5.Touch.isEnabled()) { return; }
  auto count = M5.Touch.getCount();
  for (size_t i = 0; i < count; ++i) {
    auto detail = M5.Touch.getDetail(i);
    (void)detail;
  }
}

static void test_button(void)
{
  (void)M5.BtnA.wasPressed();
  (void)M5.BtnB.isPressed();
  (void)M5.BtnC.wasReleased();
  (void)M5.BtnPWR.wasHold();
  (void)M5.BtnPWR.wasClicked();
}

static void test_power(void)
{
  M5.Power.setLed(64);
  M5.Power.setExtOutput(true);
  M5.Power.setExtPower(true);
  M5.Power.setBatteryCharge(true);
  M5.Power.setChargeCurrent(500);
  (void)M5.Power.getVBUSVoltage();
  (void)M5.Power.getBatteryLevel();
  (void)M5.Power.getBatteryVoltage();
  (void)M5.Power.isCharging();
  (void)M5.Power.getType();
}

static void test_rtc(void)
{
  if (!M5.Rtc.isEnabled()) { return; }
  m5::rtc_datetime_t dt;
  M5.Rtc.getDateTime(&dt);
  M5.Rtc.setDateTime({{ 2025, 1, 1 }, { 0, 0, 0 }});
}

static void test_log(void)
{
  M5_LOGI("build_test running on board=%d", (int)M5.getBoard());
}

void setup(void)
{
  auto cfg = M5.config();
  M5.begin(cfg);

  test_display();
  test_speaker();
  test_mic();
  test_imu();
  test_touch();
  test_button();
  test_power();
  test_rtc();
  test_log();
}

void loop(void)
{
  M5.update();
  M5.delay(10);
}

#if !defined(ARDUINO)
extern "C" {
  void loopTask(void*)
  {
    setup();
    for (;;) { loop(); }
  }
  void app_main(void)
  {
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
  }
}
#endif
