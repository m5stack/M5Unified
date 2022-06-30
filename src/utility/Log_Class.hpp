// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_Log_Class_H__
#define __M5_Log_Class_H__

#include <esp_log.h>
#include <stdarg.h>
#include <functional>


/// Output log with source info.
#ifndef M5UNIFIED_LOG_FORMAT
#define M5UNIFIED_LOG_FORMAT(letter, format) "[%6u][" #letter "][%s:%u] %s(): " format, (unsigned long) (esp_timer_get_time() / 1000ULL), m5::Log_Class::pathToFileName(__FILE__), __LINE__, __FUNCTION__
#endif

/// Output Error log with source info.
#define M5_LOGE(format, ...) M5.Log(ESP_LOG_ERROR  , M5UNIFIED_LOG_FORMAT(E, format), ##__VA_ARGS__)

/// Output Warn log with source info.
#define M5_LOGW(format, ...) M5.Log(ESP_LOG_WARN   , M5UNIFIED_LOG_FORMAT(W, format), ##__VA_ARGS__)

/// Output Info log with source info.
#define M5_LOGI(format, ...) M5.Log(ESP_LOG_INFO   , M5UNIFIED_LOG_FORMAT(I, format), ##__VA_ARGS__)

/// Output Debug log with source info.
#define M5_LOGD(format, ...) M5.Log(ESP_LOG_DEBUG  , M5UNIFIED_LOG_FORMAT(D, format), ##__VA_ARGS__)

/// Output Verbose log with source info.
#define M5_LOGV(format, ...) M5.Log(ESP_LOG_VERBOSE, M5UNIFIED_LOG_FORMAT(V, format), ##__VA_ARGS__)

namespace m5
{
  enum log_target_t : uint8_t
  {
    log_target_serial,
    log_target_display,
    log_target_callback,
    log_target_max,
  };

  class Log_Class
  {
  public:
    static const char* pathToFileName(const char * path);

    /// Output regardless of log level setting.
    void printf(const char* format, ...);
 
    /// Set whether or not to change the color for each log level.
    void setColorSerial(bool enable) { _color_serial = enable; }

    /// Set whether or not to change the color for each log level.
    void setColorDisplay(bool enable) { _color_display = enable; }

    /// Set the text to be added to the end of the log.
    void setSuffix(log_target_t target, const char* suffix) { _suffix[target] = suffix; }
    void setSuffixSerial(  const char* suffix) { _suffix[log_target_serial] = suffix; }
    void setSuffixDisplay( const char* suffix) { _suffix[log_target_display] = suffix; }
    void setSuffixCallback(const char* suffix) { _suffix[log_target_callback] = suffix; }

    /// Output specified level log.
    void log(esp_log_level_t level, bool use_suffix, const char* format, ...);

    /// Output log.
    void operator() (esp_log_level_t level, const char* format, ...);

    /// Set log level.
    void setLogLevel(log_target_t target, esp_log_level_t level) { if (target < log_target_max) { _log_level[target] = level; update_level(); } }

    /// Set log level for serial output.
    void setLogLevelSerial(  esp_log_level_t level) { _log_level[log_target_serial] = level; update_level(); }

    /// Set log level for display output.
    void setLogLevelDisplay( esp_log_level_t level) { _log_level[log_target_display] = level; update_level(); }

    /// Set log level for user callback output.
    void setLogLevelCallback(esp_log_level_t level) { _log_level[log_target_callback] = level; update_level(); }

    /// Get log level for serial output.
    esp_log_level_t getLogLevel(esp_log_level_t level) const { return _log_level[level]; }

    /// Get log level for serial output.
    esp_log_level_t getLogLevelSerial(void) const { return _log_level[log_target_serial]; }

    /// Get log level for display output.
    esp_log_level_t getLogLevelDisplay(void) const { return _log_level[log_target_display]; }

    /// Get log level for user callback output.
    esp_log_level_t getLogLevelCallback(void) const { return _log_level[log_target_callback]; }

    /// set logging callback function / functor .
    /// @param function Pointer to a user-defined function that takes a const char* as an argument.
    void setCallback(std::function<void(const char*)> function) { _callback = function; };

  private:
    static constexpr const char str_crlf[3] = "\r\n";

    void output(esp_log_level_t level, bool suffix, const char* __restrict format, va_list arg);
    void update_level(void);

    std::function<void(const char*)> _callback;

#if defined ( CORE_DEBUG_LEVEL )
    esp_log_level_t _level_maximum  = (esp_log_level_t)CORE_DEBUG_LEVEL;
#else
    esp_log_level_t _level_maximum  = (esp_log_level_t)CONFIG_LOG_DEFAULT_LEVEL;
#endif
    esp_log_level_t _log_level[3]   = { _level_maximum, _level_maximum, _level_maximum };

    const char* _suffix[3] = { str_crlf, str_crlf, str_crlf };

    bool _color_serial = true;
    bool _color_display = true;
  };
}
#endif
