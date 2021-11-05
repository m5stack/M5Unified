#if defined ( ARDUINO )

 #define WIFI_SSID     "YOUR WIFI SSID NAME"
 #define WIFI_PASSWORD "YOUR WIFI PASSWORD"
 #define NTP_TIMEZONE  "JST-9"
 #define NTP_SERVER1   "ntp.nict.jp"
 #define NTP_SERVER2   "ntp.jst.mfeed.ad.jp"
 #define NTP_SERVER3   ""

 #include <WiFi.h>

#endif

#include <M5Unified.h>

void setup(void)
{
  Serial.begin(115200);

  M5.begin();

  if (!M5.Rtc.isEnabled()) // (Internal RTC not found)
  { // External RTC setup ( for Unit RTC )
    M5.Power.setExtPower(true);
    M5.Ex_I2C.begin();
    M5.Rtc.begin(&M5.Ex_I2C);
  }

  if (!M5.Rtc.isEnabled())
  { 
    Serial.println("RTC not found.");
    for (;;) { m5gfx::delay(500); }
  }

  Serial.println("RTC found.");


/* /// setup RTC ( direct setting )

  //                      YYYY  MM  DD      hh  mm  ss
  M5.Rtc.setDateTime( { { 2021, 12, 31 }, { 12, 34, 56 } } );

//*/


/* /// setup RTC ( NTP auto setting )

  WiFi.begin( WIFI_SSID, WIFI_PASSWORD );
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(500);
  }
  Serial.println("\r\n WiFi Connected.");

  time_t t;
  while ((t = time(nullptr)) < 65536)
  {
    Serial.print('.');
    delay(500);
  }
  Serial.println("\r\n NTP Connected.");

  t++; // Advance one second.
  while (t > time(nullptr));  /// Synchronization in seconds
  M5.Rtc.setDateTime( localtime( &t ) );

//*/

}

void loop(void)
{
  static constexpr const char* const wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};

  delay(500);

  auto dt = M5.Rtc.getDateTime();
  Serial.printf("RTC : %04d/%02d/%02d (%s)  %02d:%02d:%02d \r\n"
               , dt.date.year
               , dt.date.month
               , dt.date.date
               , wd[dt.date.weekDay]
               , dt.time.hours
               , dt.time.minutes
               , dt.time.seconds
               );
  M5.Display.printf("RTC : %04d/%02d/%02d (%s)  %02d:%02d:%02d \r\n"
               , dt.date.year
               , dt.date.month
               , dt.date.date
               , wd[dt.date.weekDay]
               , dt.time.hours
               , dt.time.minutes
               , dt.time.seconds
               );


/*
 /// ESP32 internal timer
  auto t = time(nullptr);
  auto tm = localtime(&t);

  Serial.printf("ESP32:%04d/%02d/%02d(%s) %02d:%02d:%02d\r\n",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        wd[tm->tm_wday],
        tm->tm_hour, tm->tm_min, tm->tm_sec);
  M5.Display.printf("ESP32:%04d/%02d/%02d(%s) %02d:%02d:%02d\r\n",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        wd[tm->tm_wday],
        tm->tm_hour, tm->tm_min, tm->tm_sec);

//*/

}
//*/

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
