// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Log_Class.hpp"

#if defined ( M5UNIFIED_PC_BUILD )
#include <iostream>
static constexpr const uint8_t log_colors_serial[] = { 98, 91, 93, 92, 96, 97, };
#else
static constexpr const uint8_t log_colors_serial[] = { 38, 31, 33, 32, 36, 37, };
#endif
static constexpr const uint8_t log_colors_display[] = { 0xFF, 0xE0, 0xFC, 0x18, 0x1F, 0x92, };

namespace m5
{
  constexpr const char Log_Class::str_crlf[3];

  const char* Log_Class::pathToFileName(const char* path)
  {
    size_t i = 0;
    size_t pos = 0;
    char * p = (char *)path;
    while(*p)
    {
      i++;
      if(*p == '/' || *p == '\\')
      {
          pos = i;
      }
      p++;
    }
    return path+pos;
  }

  void Log_Class::operator() (esp_log_level_t level, const char* format, ...)
  {
    if (_level_maximum < level) { return; }
    va_list args;
    va_start(args, format);
    output(level, true, format, args);
    va_end(args);
  }

  void Log_Class::printf(const char* format, ...)
  {
    va_list args;
    va_start(args, format);
    output(esp_log_level_t::ESP_LOG_NONE, false, format, args);
    va_end(args);
  }

  void Log_Class::update_level(void)
  {
    _level_maximum = std::max(std::max(_log_level[log_target_serial], _log_level[log_target_display]), _log_level[log_target_callback]);
  }

  void Log_Class::output(esp_log_level_t level, bool suffix, const char* __restrict format, va_list arg)
  {
    char loc_buf[64];
    char * str = loc_buf;
    va_list copy;
    va_copy(copy, arg);
    int len = vsnprintf(str, sizeof(loc_buf), format, copy);
    va_end(copy);
    if (len < 0) { return; }
    if ((size_t)len >= sizeof(loc_buf))
    {
      auto tmp = (char*) alloca(len + 1);
      if (tmp)
      {
        str = tmp;
        len = vsnprintf(str, len+1, format, arg);
      }
    }

    if (_log_level[log_target_serial] >= level)
    {
      if (level != ESP_LOG_NONE && _use_color[log_target_serial])
      {
#if defined(M5UNIFIED_PC_BUILD)
        ::printf("\033[0;%dm%s\033[0m", log_colors_serial[level], str);
#elif defined ( ARDUINO )
        log_printf("\033[0;%dm%s\033[0m", log_colors_serial[level], str);
#else
        esp_rom_printf("\033[0;%dm%s\033[0m", log_colors_serial[level], str);
#endif
      }
      else
      {
#if defined(M5UNIFIED_PC_BUILD)
        std::cout << str;
#elif defined ( ARDUINO )
        log_printf(str);
#else
        esp_rom_printf(str);
#endif
      }
      if (suffix && _suffix[log_target_serial])
      {
#if defined(M5UNIFIED_PC_BUILD)
        std::cout << _suffix[log_target_serial];
#elif defined ( ARDUINO )
        log_printf(_suffix[log_target_serial]);
#else
        esp_rom_printf(_suffix[log_target_serial]);
#endif
      }

#if defined(M5UNIFIED_PC_BUILD)
      fflush(stdout);
#endif
    }

    if (_display && _log_level[log_target_display] >= level)
    {
      if (level != ESP_LOG_NONE && _use_color[log_target_display])
      {
        auto style = _display->getTextStyle();
        if (style.fore_rgb888 == style.back_rgb888)
        {
          _display->setTextColor(log_colors_display[level]);
        }
        else
        {
          _display->setTextColor(log_colors_display[level], m5gfx::color_convert<m5gfx::rgb332_t, m5gfx::rgb888_t>(style.back_rgb888));
        }
        _display->print(str);
        _display->setTextStyle(style);
      }
      else
      {
        _display->print(str);
      }
      if (suffix && _suffix[log_target_display]) { _display->print(_suffix[log_target_display]); }
    }

    if (_log_level[log_target_callback] >= level && _callback != nullptr)
    {
      _callback(level, _log_level[log_target_callback], str);
      if (suffix && _suffix[log_target_callback]) { _callback(level, _log_level[log_target_callback], _suffix[log_target_callback]); }
    }
  }

  void Log_Class::setDisplay(M5GFX* target)
  {
    _display = target;
  }
}
