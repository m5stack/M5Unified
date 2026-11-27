#include <M5Unified.h>

void setup()
{
  M5.begin();
  if (!M5.Buzzer.isEnabled()) {
    M5_LOGW("This configuration has no buzzer output.");
  }
}

void loop()
{
  // On CoreMatrix, keep calling update() so finite tones stop on time.
  M5.update();
  static uint32_t last_tone = 0;
  const uint32_t now = M5.millis();
  if (now - last_tone >= 1000) {
    last_tone = now;
    if (M5.Buzzer.isEnabled()) {
      M5.Buzzer.tone(1000, 120);
    }
  }
  M5.delay(10);
}
