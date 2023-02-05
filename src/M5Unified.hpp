// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5UNIFIED_HPP__
#define __M5UNIFIED_HPP__

#include <sdkconfig.h>

// If you want to use a set of functions to handle SD/SPIFFS/HTTP,
//  please include <SD.h>,<SPIFFS.h>,<HTTPClient.h> before <M5GFX.h>
// #include <SD.h>
// #include <SPIFFS.h>
// #include <HTTPClient.h>

#include <M5GFX.h>

#include "gitTagVersion.h"
#include "utility/RTC8563_Class.hpp"
#include "utility/AXP192_Class.hpp"
#include "utility/IP5306_Class.hpp"
#include "utility/IMU_Class.hpp"
#include "utility/Button_Class.hpp"
#include "utility/Power_Class.hpp"
#include "utility/Speaker_Class.hpp"
#include "utility/Mic_Class.hpp"
#include "utility/Touch_Class.hpp"
#include "utility/Log_Class.hpp"

#include <memory>
#include <vector>

namespace m5
{
  using board_t = m5gfx::board_t;
  using touch_point_t = m5gfx::touch_point_t;
  using touch_detail_t = Touch_Class::touch_detail_t;

  class M5Unified
  {
  public:
    struct config_t
    {
#if defined ( ARDUINO )

      /// use "Serial" begin. (0=disabled)
      uint32_t serial_baudrate = 115200;

#endif

      /// Clear the screen when startup.
      bool clear_display = true;

      /// 5V output to external port.
      bool output_power  = true;

      /// use PMIC(AXP192) pek for M5.BtnPWR.
      bool pmic_button   = true;

      /// use internal IMU.
      bool internal_imu  = true;

      /// use internal RTC.
      bool internal_rtc  = true;

      /// use the microphone.
      bool internal_mic  = true;

      /// use the speaker.
      bool internal_spk  = true;

      /// use Unit Accel & Gyro.
      bool external_imu  = false;

      /// use Unit RTC.
      bool external_rtc  = false;

      /// system LED brightness (0=off / 255=max) (※ not NeoPixel)
      uint8_t led_brightness = 0;

      union
      {
        uint8_t external_spk = 0;
        struct
        {
          uint8_t enabled : 1;
          uint8_t omit_atomic_spk : 1;
          uint8_t omit_spk_hat : 1;
          uint8_t omit_module_display : 1;
          uint8_t omit_module_rca : 1;
          uint8_t reserve : 3;
        } external_spk_detail;
      };

      /// use Unit OLED / Unit LCD / Atom Display / Module Display
      /// (※ Unit RCA / Module RCA is impossible to auto detection)
      union
      {
        uint8_t external_dsp = 1;
        struct
        {
          uint8_t enabled : 1;
          uint8_t omit_module_rca : 1;
          uint8_t omit_unit_rca : 1;
          uint8_t reserve : 5;
        } external_dsp_detail;
      };
    };

    config_t config(void) const { return _cfg; }

    void config(const config_t& cfg) { _cfg = cfg; }

    /// get the board type of the runtime environment.
    /// @return board type
    board_t getBoard(void) const { return _board; }

    /// Perform initialization process at startup.
    void begin(const config_t& cfg)
    {
      _cfg = cfg;
      begin();
    }

    void begin(void)
    {
      auto brightness = _primaryDisplay.getBrightness();
      _primaryDisplay.setBrightness(0);
      bool res = _primaryDisplay.init_without_reset();
      _board = _check_boardtype(_primaryDisplay.getBoard());
      if (res && getDisplayCount() == 0) {
        addDisplay(_primaryDisplay);
      }

#if defined ( __M5GFX_M5ATOMDISPLAY__ )
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3)
      if (_cfg.external_dsp) {
        if (_board == board_t::board_M5Atom || _board == board_t::board_M5AtomPsram || _board == board_t::board_M5AtomS3 || _board == board_t::board_M5AtomS3Lite)
        {
          M5AtomDisplay dsp;
          if (dsp.init_without_reset()) {
            addDisplay(dsp);
          }
        }
      }
#endif
#endif

      _begin();

      // Module Display / Unit OLED / Unit LCD is determined after _begin (because it must be after external power supply)

      if (_cfg.external_dsp) {

#if defined ( __M5GFX_M5MODULEDISPLAY__ )
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3)
        if (_board == board_t::board_M5Stack || _board == board_t::board_M5StackCore2 || _board == board_t::board_M5Tough || _board == board_t::board_M5StackCoreS3)
        {
          M5ModuleDisplay dsp;
          if (dsp.init()) {
            addDisplay(dsp);
          }
        }
#endif
#endif

#if defined ( __M5GFX_M5UNITOLED__ )
        {
          M5UnitOLED dsp = { (uint8_t)Ex_I2C.getSDA(), (uint8_t)Ex_I2C.getSCL(), 400000, (int8_t)Ex_I2C.getPort() };
          if (dsp.init()) {
            addDisplay(dsp);
          }
        }
#endif

#if defined ( __M5GFX_M5UNITLCD__ )
        {
          M5UnitLCD dsp = { (uint8_t)Ex_I2C.getSDA(), (uint8_t)Ex_I2C.getSCL(), 400000, (int8_t)Ex_I2C.getPort() };
          int retry = 8;
          do {
            delay(32);
            if (dsp.init()) {
              addDisplay(dsp);
              break;
            }
          } while (--retry);
        }
#endif

// RCAはESP32S3では使用できない
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
#if defined ( __M5GFX_M5MODULERCA__ )
        {
          M5ModuleRCA dsp;
          if (dsp.init()) {
            addDisplay(dsp);
          }
        }
#elif defined ( __M5GFX_M5UNITRCA__ )
        {
          M5UnitRCA dsp;
          if (dsp.init()) {
            addDisplay(dsp);
          }
        }
#endif
#endif

      }

      // Speaker selection is performed after the Module Display has been determined.
      _begin_spk();

      _primaryDisplay.setBrightness(brightness);

      update();
    }

    /// To call this function in a loop function.
    void update(void);

    /// milli seconds at the time the update was called
    std::uint32_t getUpdateMsec(void) const { return _updateMsec; }

    M5GFX &Display = _primaryDisplay;
    M5GFX &Lcd = _primaryDisplay;

    IMU_Class Imu;
    Log_Class Log;
    Power_Class Power;
    RTC8563_Class Rtc;
    Touch_Class Touch;

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
    Button_Class BtnA;
    Button_Class BtnB;
    Button_Class BtnC;
    Button_Class BtnEXT;  // CoreInk top button
    Button_Class BtnPWR;  // CoreInk power button / AXP192 power button

    /// for internal I2C device
    I2C_Class& In_I2C = m5::In_I2C;

    /// for external I2C device (Port.A)
    I2C_Class& Ex_I2C = m5::Ex_I2C;

    Speaker_Class Speaker;

    Mic_Class Mic;

    M5GFX& Displays(size_t index) { return index == _primary_display_index ? _primaryDisplay : this->_displays[index]; }

    std::size_t getDisplayCount(void) const { return this->_displays.size(); }

    M5GFX& getDisplay(size_t index) { return index == _primary_display_index ? _primaryDisplay : this->_displays[index]; }

    std::size_t addDisplay(M5GFX& dsp) {
      this->_displays.push_back(dsp);
      auto res = this->_displays.size() - 1;
      setPrimaryDisplay(res == 0 ? 0 : _primary_display_index);

      // Touch screen operation is always limited to the first display.
      Touch.begin(_displays.front().touch() ? &_displays.front() : nullptr);

      return res;
    }

    bool setPrimaryDisplay(std::size_t index) {
      if (index >= _displays.size()) { return false; }
      std::size_t pdi = _primary_display_index;
      // if (pdi == index) { return true; }
      if (pdi < _displays.size()) {
        _displays[pdi] = _primaryDisplay;
      }
      _primary_display_index = index;
      _primaryDisplay = _displays[index];
      return true;
    }

  private:
    static constexpr std::size_t BTNPWR_MIN_UPDATE_MSEC = 4;

    std::uint32_t _updateMsec = 0;
    config_t _cfg;
    m5gfx::board_t _board = m5gfx::board_t::board_unknown;

    M5GFX _primaryDisplay;  // setPrimaryされたディスプレイのインスタンス
    std::vector<M5GFX> _displays; // 登録された全ディスプレイのインスタンス
    std::uint8_t _primary_display_index = -1;

    void _begin(void);
    void _begin_spk(void);
    board_t _check_boardtype(board_t);

    static bool _speaker_enabled_cb(void* args, bool enabled);
    static bool _microphone_enabled_cb(void* args, bool enabled);
  };
}

extern m5::M5Unified M5;

#endif