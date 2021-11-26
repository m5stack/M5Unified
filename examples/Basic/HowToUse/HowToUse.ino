
#if defined ( ARDUINO )

#include <Arduino.h>

// If you use SD card, write this.
#include <SD.h>

// If you use SPIFFS, write this.
#include <SPIFFS.h>

#endif

// * The filesystem header must be included before the display library.

//----------------------------------------------------------------

// If you use ATOMDisplay, write this.
// #define M5ATOMDISPLAY_LOGICAL_WIDTH  1280  // width
// #define M5ATOMDISPLAY_LOGICAL_HEIGHT  720  // height
// #define M5ATOMDISPLAY_REFRESH_RATE     60  // refresh rate
#include <M5AtomDisplay.h>

// If you use Unit LCD, write this.
#include <M5UnitLCD.h>

// If you use Unit OLED, write this.
#include <M5UnitOLED.h>

// * The display header must be included before the M5Unified library.

//----------------------------------------------------------------

// Include this to enable the M5 global instance.
#include <M5Unified.h>


#include <esp_log.h>

void setup(void)
{
  auto cfg = M5.config();

#if defined ( ARDUINO )
  cfg.serial_baudrate = 115200;   // default=115200. if "Serial" is not needed, set it to 0.
#endif
  cfg.clear_display = true;  // default=true. clear the screen when begin.
  cfg.output_power  = true;  // default=true. use external port 5V output.
  cfg.internal_imu  = true;  // default=true. use internal IMU.
  cfg.internal_rtc  = true;  // default=true. use internal RTC.
  cfg.external_imu  = true;  // default=false. use Unit Accel & Gyro.
  cfg.external_rtc  = true;  // default=false. use Unit RTC.
  cfg.led_brightness = 64;   // default= 0. system LED brightness (0=off / 255=max) (※ not NeoPixel)

  M5.begin(cfg);



  if (M5.Rtc.isEnabled())
  {
//  rtc direct setting.    YYYY  MM  DD      hh  mm  ss
//  M5.Rtc.setDateTime( {{ 2021, 12, 31 }, { 12, 34, 56 }} );
  }

  /// For models with EPD : refresh control
  M5.Display.setEpdMode(epd_mode_t::epd_fastest); // fastest but very-low quality.
//M5.Display.setEpdMode(epd_mode_t::epd_fast   ); // fast but low quality.
//M5.Display.setEpdMode(epd_mode_t::epd_text   ); // slow but high quality. (for text)
//M5.Display.setEpdMode(epd_mode_t::epd_quality); // slow but high quality. (for image)

  /// For models with LCD : backlight control (0~255)
  M5.Display.setBrightness(128);

  if (M5.Display.width() < M5.Display.height())
  { /// Landscape mode.
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }

  int textsize = M5.Display.height() / 160;
  if (textsize == 0) { textsize = 1; }
  M5.Display.setTextSize(textsize);

  // run-time branch : hardware model check
  const char* name;
  switch (M5.getBoard())
  {
  case m5::board_t::board_M5Stack:
    name = "M5Stack";
    break;
  case m5::board_t::board_M5StackCore2:
    name = "M5StackCore2";
    break;
  case m5::board_t::board_M5StickC:
    name = "M5StickC";
    break;
  case m5::board_t::board_M5StickCPlus:
    name = "M5StickC-Plus";
    break;
  case m5::board_t::board_M5StackCoreInk:
    name = "M5StackCoreInk";
    break;
  case m5::board_t::board_M5Paper:
    name = "M5Paper";
    break;
  case m5::board_t::board_M5Tough:
    name = "M5Tough";
    break;
  case m5::board_t::board_M5Station:
    name = "M5Station";
    break;
  case m5::board_t::board_M5ATOM:
    name = "M5ATOM";
    break;
  case m5::board_t::board_M5TimerCam:
    name = "TimerCamera";
    break;
  default:
    name = "Who am I ?";
    break;
  }
  M5.Display.startWrite();
  M5.Display.print("Core:");
  M5.Display.println(name);

  // run-time branch : imu model check
  switch (M5.Imu.getType())
  {
  case m5::imu_t::imu_mpu6050:
    name = "MPU6050";
    break;
  case m5::imu_t::imu_mpu6886:
    name = "MPU6886";
    break;
  case m5::imu_t::imu_mpu9250:
    name = "MPU9250";
    break;
  case m5::imu_t::imu_sh200q:
    name = "SH200Q";
    break;
  default:
    name = "none";
    break;
  }
  M5.Display.print("IMU:");
  M5.Display.println(name);
  M5.Display.endWrite();
}

void loop(void)
{
  vTaskDelay(1);
  int h = M5.Display.height() / 8;

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
  M5ATOM:                      BtnA
*/

  static constexpr const int colors[] = { TFT_WHITE, TFT_CYAN, TFT_RED, TFT_YELLOW, TFT_BLUE };
  static constexpr const char* const names[] = { "none", "wasHold", "wasClicked", "wasPressed", "wasReleased" };

  /// BtnPWR: can "wasClicked"/"wasHold"  can be use.
  int state = M5.BtnPWR.wasHold() ? 1
            : M5.BtnPWR.wasClicked() ? 2
            : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnPWR:%s", names[state]);
    M5.Display.fillRect(0, h*2, h, h-1, colors[state]);
  }

  /// BtnA,BtnB,BtnC,BtnEXT, BtnPWR of CoreInk: "isPressed"/"wasPressed"/"isReleased"/"wasReleased"/"wasClicked"/"wasHold"/"isHolding"  can be use.
  state = M5.BtnA.wasHold() ? 1
        : M5.BtnA.wasClicked() ? 2
        : M5.BtnA.wasPressed() ? 3
        : M5.BtnA.wasReleased() ? 4
        : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnA:%s", names[state]);
    M5.Display.fillRect(0, h*3, h, h-1, colors[state]);
  }

  state = M5.BtnB.wasHold() ? 1
        : M5.BtnB.wasClicked() ? 2
        : M5.BtnB.wasPressed() ? 3
        : M5.BtnB.wasReleased() ? 4
        : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnB:%s", names[state]);
    M5.Display.fillRect(0, h*4, h, h-1, colors[state]);
  }

  state = M5.BtnC.wasHold() ? 1
        : M5.BtnC.wasClicked() ? 2
        : M5.BtnC.wasPressed() ? 3
        : M5.BtnC.wasReleased() ? 4
        : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnC:%s", names[state]);
    M5.Display.fillRect(0, h*5, h, h-1, colors[state]);
  }

  state = M5.BtnEXT.wasHold() ? 1
        : M5.BtnEXT.wasClicked() ? 2
        : M5.BtnEXT.wasPressed() ? 3
        : M5.BtnEXT.wasReleased() ? 4
        : 0;
  if (state)
  {
    ESP_LOGI("loop", "BtnEXT:%s", names[state]);
    M5.Display.fillRect(0, h*6, h, h-1, colors[state]);
  }

  static uint32_t prev_sec;
  uint32_t sec = m5gfx::millis() / 1000;
  if (prev_sec != sec)
  {
    prev_sec = sec;

//------------------- Battery level
    static int prev_battery = INT_MAX;
    int battery = M5.Power.getBatteryLevel();
    if (prev_battery != battery)
    {
      prev_battery = battery;
      M5.Display.startWrite();
      M5.Display.setCursor(0, M5.Display.fontHeight() * 2);
      M5.Display.print("Bat:");
      if (battery >= 0)
      {
        M5.Display.printf("%03d", battery);
      }
      else
      {
        M5.Display.print("none");
      }
      M5.Display.endWrite();
    }
//------------------- RTC test
    if (M5.Rtc.isEnabled())
    {
      static constexpr const char* const wd[] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat","ERR"};
      char buf[32];
//*
      /// Get the date and time from the RTC and display it.
      m5::rtc_datetime_t dt;
      if (M5.Rtc.getDateTime(&dt))
      {
        M5.Display.startWrite();
        snprintf( buf, 30, "%04d/%02d/%02d(%s)"
                , dt.date.year
                , dt.date.month
                , dt.date.date
                , wd[dt.date.weekDay & 7]
                );
        M5.Display.drawString(buf, M5.Display.width() / 2, 0);
        snprintf( buf, 30, "%02d:%02d:%02d"
                , dt.time.hours
                , dt.time.minutes
                , dt.time.seconds
                );
        M5.Display.drawString(buf, M5.Display.width() / 2, M5.Display.fontHeight());
        M5.Display.endWrite();
      }
      else
      {
        M5.Display.drawString("RTC error", M5.Display.width() / 2, M5.Display.fontHeight()>>1);
      }
/*/
/// In the example above, the date and time are obtained through I2C communication with the RTC.
/// However, since M5Unified synchronizes the ESP32's internal clock at startup, 
/// it is also possible to get the date and time, as shown in the example below.
/// ※ Note that there will be an error of a few seconds per day. 
///    You may want to call M5.Rtc.setSystemTimeFromRtc() periodically to synchronize.
      auto t = time(nullptr);
      auto time = localtime(&t);
      M5.Display.startWrite();
      snprintf( buf, 30, "%04d/%02d/%02d(%s)"
              , time->tm_year + 1900
              , time->tm_mon + 1
              , time->tm_mday
              , wd[time->tm_wday & 7]
              );
      M5.Display.drawString(buf, M5.Display.width() / 2, 0);
      snprintf( buf, 30, "%02d:%02d:%02d"
              , time->tm_hour
              , time->tm_min
              , time->tm_sec
              );
      M5.Display.drawString(buf, M5.Display.width() / 2, M5.Display.fontHeight());
      M5.Display.endWrite();
//*/
    }
  }

//------------------- IMU test
  if (M5.Imu.isEnabled())
  {
    int ox = (M5.Display.width()+h)>>1;
    static int prev_xpos[6];
    int xpos[6];
    float val[6];
    M5.Imu.getAccel(&val[0], &val[1], &val[2]);
    M5.Imu.getGyro(&val[3], &val[4], &val[5]);
    int color[6] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_RED, TFT_GREEN, TFT_BLUE };

    for (int i = 0; i < 3; ++i)
    {
      xpos[i]   = val[i] * 50;
      xpos[i+3] = val[i+3] / 2;
    }

    M5.Display.startWrite();
    M5.Display.setClipRect(h, h, M5.Display.width(), M5.Display.height());
    M5.Display.waitDisplay();
    for (int i = 0; i < 6; ++i)
    {
      if (xpos[i] == prev_xpos[i]) continue;

      int px = prev_xpos[i];
      if ((xpos[i] < 0) != (px < 0))
      {
        if (px)
        {
          M5.Display.fillRect(ox, h * (i+2), px, h, M5.Display.getBaseColor());
        }
        px = 0;
      }
      if (xpos[i] != px)
      {
        if ((xpos[i] > px) != (xpos[i] < 0))
        {
          M5.Display.setColor(color[i]);
        }
        else
        {
          M5.Display.setColor(M5.Display.getBaseColor());
        }
        M5.Display.fillRect(xpos[i] + ox, h * (i+2), px - xpos[i], h);
      }
      prev_xpos[i] = xpos[i];
    }
    M5.Display.clearClipRect();

    M5.Display.endWrite();
  }
  M5.Display.display();
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
