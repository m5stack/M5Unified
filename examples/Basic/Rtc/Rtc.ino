#if defined ( ARDUINO )

 #define WIFI_SSID     "YOUR WIFI SSID NAME"
 #define WIFI_PASSWORD "YOUR WIFI PASSWORD"
 #define NTP_TIMEZONE  "JST-9"
 #define NTP_SERVER1   "0.pool.ntp.org"
 #define NTP_SERVER2   "1.pool.ntp.org"
 #define NTP_SERVER3   "2.pool.ntp.org"

 #include <WiFi.h>

// Different versions of the framework have different SNTP header file names and availability.
 #if __has_include (<esp_sntp.h>)
  #include <esp_sntp.h>
  #define SNTP_ENABLED 1
 #elif __has_include (<sntp.h>)
  #include <sntp.h>
  #define SNTP_ENABLED 1
 #endif

#endif

#ifndef SNTP_ENABLED
#define SNTP_ENABLED 0
#endif

#include <M5Unified.h>

void setup(void)
{
  auto cfg = M5.config();

  cfg.external_rtc  = true;  // default=false. use Unit RTC.

  M5.begin(cfg);

  M5.Display.setEpdMode(m5gfx::epd_fastest);
  M5.setLogDisplayIndex(0);

  if (!M5.Rtc.isEnabled())
  {
    M5.Log.println("RTC not found.");
    for (;;) { M5.delay(500); }
  }

  M5.Log.println("RTC found.");

// It is recommended to set UTC for the RTC and ESP32 internal clocks.


/* /// setup RTC ( direct setting )
  //                      YYYY  MM  DD      hh  mm  ss
  M5.Rtc.setDateTime( { { 2021, 12, 31 }, { 12, 34, 56 } } );
//*/


/// setup RTC ( NTP auto setting )
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
#ifdef WiFi_h
  M5.Log.print("WiFi:");
  WiFi.begin( WIFI_SSID, WIFI_PASSWORD );

  for (int i = 20; i && WiFi.status() != WL_CONNECTED; --i)
  {
    M5.Log.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    M5.Log.println("\r\nWiFi Connected.");
    M5.Log.print("NTP:");
#if SNTP_ENABLED
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
    {
      M5.Log.print(".");
      delay(1000);
    }
#else
    delay(1600);
    struct tm timeInfo;
    while (!getLocalTime(&timeInfo, 1000))
    {
      M5.Log.print('.');
    };
#endif
    M5.Log.println("\r\nNTP Connected.");

    time_t t = time(nullptr)+1; // Advance one second.
    while (t > time(nullptr));  /// Synchronization in seconds
    M5.Rtc.setDateTime( gmtime( &t ) );
  }
  else
  {
    M5.Log.println("\r\nWiFi none...");
  }
#endif

  M5.Display.clear();
}

void loop(void)
{
  static constexpr const char* const wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};

  delay(500);

  auto dt = M5.Rtc.getDateTime();
  M5.Display.setCursor(0,0);
  M5.Log.printf("RTC   UTC  :%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n"
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
  {
    auto tm = gmtime(&t);    // for UTC.
    M5.Display.setCursor(0,20);
    M5.Log.printf("ESP32 UTC  :%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n",
          tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
          wd[tm->tm_wday],
          tm->tm_hour, tm->tm_min, tm->tm_sec);
  }

  {
    auto tm = localtime(&t); // for local timezone.
    M5.Display.setCursor(0,40);
    M5.Log.printf("ESP32 %s:%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n", NTP_TIMEZONE,
          tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
          wd[tm->tm_wday],
          tm->tm_hour, tm->tm_min, tm->tm_sec);
  }
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
