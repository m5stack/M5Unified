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
  auto cfg = M5.config();

  cfg.external_rtc  = true;  // default=false. use Unit RTC.

  M5.begin(cfg);

  if (!M5.Rtc.isEnabled())
  {
    Serial.println("RTC not found.");
    M5.Display.println("RTC not found.");
    for (;;) { vTaskDelay(500); }
  }

  Serial.println("RTC found.");


/* /// setup RTC ( direct setting )

  //                      YYYY  MM  DD      hh  mm  ss
  M5.Rtc.setDateTime( { { 2021, 12, 31 }, { 12, 34, 56 } } );

//*/


/* /// setup RTC ( NTP auto setting )

  M5.Display.print("WiFi:");
  WiFi.begin( WIFI_SSID, WIFI_PASSWORD );
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(500);
  }
  Serial.println("\r\n WiFi Connected.");
  M5.Display.print("Connected.");

  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("\r\n NTP Connected.");

  time_t t = time(nullptr)+1; // Advance one second.
  while (t > time(nullptr));  /// Synchronization in seconds
  M5.Rtc.setDateTime( localtime( &t ) );

//*/

}

void loop(void)
{
  static constexpr const char* const wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};

  delay(500);

  auto dt = M5.Rtc.getDateTime();
  Serial.printf("RTC : %04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n"
               , dt.date.year
               , dt.date.month
               , dt.date.date
               , wd[dt.date.weekDay]
               , dt.time.hours
               , dt.time.minutes
               , dt.time.seconds
               );
  M5.Display.setCursor(0,0);
  M5.Display.printf("RTC : %04d/%02d/%02d (%s)  %02d:%02d:%02d"
               , dt.date.year
               , dt.date.month
               , dt.date.date
               , wd[dt.date.weekDay]
               , dt.time.hours
               , dt.time.minutes
               , dt.time.seconds
               );


 /// ESP32 internal timer
  auto t = time(nullptr);
  auto tm = localtime(&t);

  Serial.printf("ESP32:%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        wd[tm->tm_wday],
        tm->tm_hour, tm->tm_min, tm->tm_sec);
  M5.Display.setCursor(0,20);
  M5.Display.printf("ESP32:%04d/%02d/%02d (%s)  %02d:%02d:%02d",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        wd[tm->tm_wday],
        tm->tm_hour, tm->tm_min, tm->tm_sec);

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
