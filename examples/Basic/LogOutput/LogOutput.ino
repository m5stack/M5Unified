#include <M5Unified.h>


void user_made_log_callback(esp_log_level_t, bool, const char*);


void setup(void)
{
/// Example of ESP32 standard log output macro usage.
/// ESP_LOGx series outputs to USB-connected PC terminal.
/// You can set the output level with `Core Debug Level` in ArduinoIDE.
/// ※ If the `Core Debug Level` is set to `None`, nothing is output.
#if defined ( ESP_PLATFORM )
  ESP_LOGE("TAG", "using ESP_LOGE error log.");    // Critical errors, software module can not recover on its own
  ESP_LOGW("TAG", "using ESP_LOGW warn log.");     // Error conditions from which recovery measures have been taken
  ESP_LOGI("TAG", "using ESP_LOGI info log.");     // Information messages which describe normal flow of events
  ESP_LOGD("TAG", "using ESP_LOGD debug log.");    // Extra information which is not necessary for normal use (values, pointers, sizes, etc).
  ESP_LOGV("TAG", "using ESP_LOGV verbose log.");  // Bigger chunks of debugging information, or frequent messages which can potentially flood the output.
#endif

  M5.begin();

  /// If you want to output logs to the display, write this.
  M5.setLogDisplayIndex(0);

  /// use wrapping from bottom edge to top edge.
  M5.Display.setTextWrap(true, true);
/// use scrolling.
// M5.Display.setTextScroll(true);

/// Example of M5Unified log output class usage.
/// Unlike ESP_LOGx, the M5.Log series can output to serial, display, and user callback function in a single line of code.


/// You can set Log levels for each output destination.
/// ESP_LOG_ERROR / ESP_LOG_WARN / ESP_LOG_INFO / ESP_LOG_DEBUG / ESP_LOG_VERBOSE
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_VERBOSE);
  M5.Log.setLogLevel(m5::log_target_display, ESP_LOG_DEBUG);
  M5.Log.setLogLevel(m5::log_target_callback, ESP_LOG_INFO);

/// Set up user-specific callback functions.
  M5.Log.setCallback(user_made_log_callback);

/// You can color the log or not.
  M5.Log.setEnableColor(m5::log_target_serial, true);
  M5.Log.setEnableColor(m5::log_target_display, true);
  M5.Log.setEnableColor(m5::log_target_callback, true);

/// You can set the text to be added to the end of the log for each output destination.
/// ( default value : "\n" )
  M5.Log.setSuffix(m5::log_target_serial, "\n");
  M5.Log.setSuffix(m5::log_target_display, "\n");
  M5.Log.setSuffix(m5::log_target_callback, "");

/// `M5.Log()` can be used to output a simple log
  M5.Log(ESP_LOG_ERROR   , "M5.Log error log");    /// ERROR level output
  M5.Log(ESP_LOG_WARN    , "M5.Log warn log");     /// WARN level output
  M5.Log(ESP_LOG_INFO    , "M5.Log info log");     /// INFO level output
  M5.Log(ESP_LOG_DEBUG   , "M5.Log debug log");    /// DEBUG level output
  M5.Log(ESP_LOG_VERBOSE , "M5.Log verbose log");  /// VERBOSE level output

/// `M5_LOGx` macro can be used to output a log containing the source file name, line number, and function name.
  M5_LOGE("M5_LOGE error log");     /// ERROR level output with source info
  M5_LOGW("M5_LOGW warn log");      /// WARN level output with source info
  M5_LOGI("M5_LOGI info log");      /// INFO level output with source info
  M5_LOGD("M5_LOGD debug log");     /// DEBUG level output with source info
  M5_LOGV("M5_LOGV verbose log");   /// VERBOSE level output with source info

/// `M5.Log.printf()` is output without log level and without suffix and is output to all serial, display, and callback.
  M5.Log.printf("M5.Log.printf non level output\n");
}

void loop(void)
{
  M5.delay(1);
  M5.update();

  if (M5.BtnPWR.wasClicked()) { M5_LOGE("BtnP %d click", M5.BtnPWR.getClickCount()); }
  if (M5.BtnA  .wasClicked()) { M5_LOGW("BtnA %d click", M5.BtnA  .getClickCount()); }
  if (M5.BtnB  .wasClicked()) { M5_LOGI("BtnB %d click", M5.BtnB  .getClickCount()); }
  if (M5.BtnC  .wasClicked()) { M5_LOGD("BtnC %d click", M5.BtnC  .getClickCount()); }

  static uint32_t counter = 0;
  if ((++counter & 0x3FF) == 0)
  {
    static int prev_y;
    int cursor_y = M5.Display.getCursorY();
    if (prev_y > cursor_y)
    {
      M5.Display.clear();
    }
    prev_y = cursor_y;
    M5_LOGV("count:%d", counter >> 10);  
  }
}

void user_made_log_callback(esp_log_level_t log_level, bool use_color, const char* log_text)
{
// You can also create your own callback function to output log contents to a file,WiFi,and more other destination

#if defined ( ARDUINO )
/*
  if (SD.begin(GPIO_NUM_4, SPI, 25000000))
  {
    auto file = SD.open("/logfile.txt", FILE_APPEND);
    file.print(log_text);
    file.close();
    SD.end();
  }
//*/
#endif

}

/// for ESP-IDF
#if !defined ( ARDUINO ) && defined ( ESP_PLATFORM )
extern "C"
{
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
