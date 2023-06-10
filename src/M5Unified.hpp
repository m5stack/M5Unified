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

namespace m5
{
  using board_t = m5gfx::board_t;
  using touch_point_t = m5gfx::touch_point_t;
};

#include "gitTagVersion.h"
#include "utility/RTC8563_Class.hpp"
#include "utility/AXP192_Class.hpp"
#include "utility/IP5306_Class.hpp"
#include "utility/Button_Class.hpp"
#include "utility/Power_Class.hpp"
#include "utility/Speaker_Class.hpp"
#include "utility/Mic_Class.hpp"
#include "utility/Touch_Class.hpp"
#include "utility/Log_Class.hpp"
#include "utility/IMU_Class.hpp"

#include <memory>
#include <vector>

namespace m5
{
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

      union
      {
        struct
        {
          uint8_t module_display : 1;
          uint8_t module_rca : 1;
          uint8_t hat_spk : 1;
          uint8_t atomic_spk : 1;
          uint8_t hat_spk2 : 1;
          uint8_t reserve : 3;
        } external_speaker;
        uint8_t external_speaker_value = 0x00;
      };

      union
      {
        struct
        {
          uint8_t module_display : 1;
          uint8_t atom_display : 1;
          uint8_t unit_oled : 1;
          uint8_t unit_lcd : 1;
          uint8_t unit_glass : 1;
          uint8_t unit_rca : 1;
          uint8_t module_rca : 1;
          uint8_t reserve : 1;
        } external_display;
        uint8_t external_display_value = 0xFF;
      };

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

      /// If auto-detection fails, the board will operate as the board configured here.
      board_t fallback_board
#if defined (CONFIG_IDF_TARGET_ESP32S3)
                             = board_t::board_M5AtomS3Lite;
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
                             = board_t::board_M5StampC3;
#elif defined (CONFIG_IDF_TARGET_ESP32) || !defined (CONFIG_IDF_TARGET)
                             = board_t::board_M5Atom;
#else
                             = board_t::board_unknown;
#endif

      union
      {
        uint8_t external_spk = 0;
        [[deprecated("Change to external_speaker")]]
        struct
        {
          uint8_t enabled : 1;
          uint8_t omit_atomic_spk : 1;
          uint8_t omit_spk_hat : 1;
          uint8_t reserve : 5;
        } external_spk_detail;
      };

#if defined ( __M5GFX_M5ATOMDISPLAY__ )
      M5AtomDisplay::config_t atom_display;
#endif
#if defined ( __M5GFX_M5MODULEDISPLAY__ )
      M5ModuleDisplay::config_t module_display;
#endif
#if defined ( __M5GFX_M5MODULERCA__ )
      M5ModuleRCA::config_t module_rca;
#endif
#if defined ( __M5GFX_M5UNITGLASS__ )
      M5UnitGLASS::config_t unit_glass;
#endif
#if defined ( __M5GFX_M5UNITOLED__ )
      M5UnitOLED::config_t unit_oled;
#endif
#if defined ( __M5GFX_M5UNITLCD__ )
      M5UnitLCD::config_t unit_lcd;
#endif
#if defined ( __M5GFX_M5UNITRCA__ )
      M5UnitRCA::config_t unit_rca;
#endif
    };

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

    M5GFX& getDisplay(size_t index);

    M5GFX& Displays(size_t index) { return getDisplay(index); }

    std::size_t getDisplayCount(void) const { return this->_displays.size(); }

    std::size_t addDisplay(M5GFX& dsp);

    // Get the display index of the type matching the argument.
    // Returns -1 if not found.
    int32_t getDisplayIndex(m5gfx::board_t board);

    int32_t getDisplayIndex(std::initializer_list<m5gfx::board_t> board_list);

    // Designates the display of the specified index as PrimaryDisplay.
    bool setPrimaryDisplay(std::size_t index);

    // Find a display that matches the specified display type and designate it as PrimaryDisplay.
    // Multiple display types can be specified in the initializer list.
    bool setPrimaryDisplayType(std::initializer_list<m5gfx::board_t> board_list);

    // Find a display that matches the specified display type and designate it as PrimaryDisplay.
    bool setPrimaryDisplayType(m5gfx::board_t board) { return setPrimaryDisplayType( { board } ); };

    /// Set the display to show logs.
    void setLogDisplayIndex(size_t index);

    void setLogDisplayType(std::initializer_list<m5gfx::board_t> board_list);

    void setLogDisplayType(m5gfx::board_t board) { setLogDisplayType( { board } ); };

    /// milli seconds at the time the update was called
    std::uint32_t getUpdateMsec(void) const { return _updateMsec; }

    static inline config_t config(void)
    {
      return config_t();
    }

    /// get the board type of the runtime environment.
    /// @return board type
    board_t getBoard(void) const { return _board; }

    /// To call this function in a loop function.
    void update(void);

    /// Perform initialization process at startup.
    void begin(void)
    {
      if (_board != m5gfx::board_t::board_unknown) { return; }
      config_t cfg;
      begin(cfg);
    }

    /// Perform initialization process at startup.
    void begin(config_t cfg)
    {
      // Allow begin execution only once.
      if (_board != m5gfx::board_t::board_unknown) { return; }

      auto brightness = _primaryDisplay.getBrightness();
      _primaryDisplay.setBrightness(0);
      bool res = _primaryDisplay.init_without_reset();
      auto board = _check_boardtype(_primaryDisplay.getBoard());
      if (board == board_t::board_unknown) { board = cfg.fallback_board; }
      _board = board;
      _setup_i2c(board);
      if (res && getDisplayCount() == 0) {
        addDisplay(_primaryDisplay);
      }

#if defined ( __M5GFX_M5ATOMDISPLAY__ )
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3)
      if (cfg.external_display.atom_display) {
        if (_board == board_t::board_M5Atom || _board == board_t::board_M5AtomPsram || _board == board_t::board_M5AtomS3 || _board == board_t::board_M5AtomS3Lite)
        {
          M5AtomDisplay dsp(cfg.atom_display);
          if (dsp.init_without_reset()) {
            addDisplay(dsp);
          }
        }
      }
#endif
#endif

      _begin(cfg);


      // Module Display / Unit OLED / Unit LCD is determined after _begin (because it must be after external power supply)
#if defined ( __M5GFX_M5MODULEDISPLAY__ )
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3)
      if (cfg.external_display.module_display) {
        if (_board == board_t::board_M5Stack || _board == board_t::board_M5StackCore2 || _board == board_t::board_M5Tough || _board == board_t::board_M5StackCoreS3)
        {
          M5ModuleDisplay dsp(cfg.module_display);
          if (dsp.init()) {
            addDisplay(dsp);
          }
        }
      }
#endif
#endif

      // Speaker selection is performed after the Module Display has been determined.
      _begin_spk(cfg);

      bool port_a_used = _begin_rtc_imu(cfg);
      (void)port_a_used;

      if (cfg.external_display_value)
      {
#if defined ( __M5GFX_M5UNITOLED__ )
        if (cfg.external_display.unit_oled)
        {
          if (cfg.unit_oled.pin_sda >= GPIO_NUM_MAX) { cfg.unit_oled.pin_sda = (uint8_t)Ex_I2C.getSDA(); }
          if (cfg.unit_oled.pin_scl >= GPIO_NUM_MAX) { cfg.unit_oled.pin_scl = (uint8_t)Ex_I2C.getSCL(); }
          if (cfg.unit_oled.i2c_port < 0) { cfg.unit_oled.i2c_port = (int8_t)Ex_I2C.getPort(); }

          M5UnitOLED dsp(cfg.unit_oled);
          if (dsp.init()) {
            addDisplay(dsp);
            port_a_used = true;
          }
        }
#endif

#if defined ( __M5GFX_M5UNITGLASS__ )
        if (cfg.external_display.unit_glass)
        {
          if (cfg.unit_glass.pin_sda >= GPIO_NUM_MAX) { cfg.unit_glass.pin_sda = (uint8_t)Ex_I2C.getSDA(); }
          if (cfg.unit_glass.pin_scl >= GPIO_NUM_MAX) { cfg.unit_glass.pin_scl = (uint8_t)Ex_I2C.getSCL(); }
          if (cfg.unit_glass.i2c_port < 0) { cfg.unit_glass.i2c_port = (int8_t)Ex_I2C.getPort(); }

          M5UnitGLASS dsp(cfg.unit_glass);
          if (dsp.init()) {
            addDisplay(dsp);
            port_a_used = true;
          }
        }
#endif

#if defined ( __M5GFX_M5UNITLCD__ )
        if (cfg.external_display.unit_lcd)
        {
          if (cfg.unit_lcd.pin_sda >= GPIO_NUM_MAX) { cfg.unit_lcd.pin_sda = (uint8_t)Ex_I2C.getSDA(); }
          if (cfg.unit_lcd.pin_scl >= GPIO_NUM_MAX) { cfg.unit_lcd.pin_scl = (uint8_t)Ex_I2C.getSCL(); }
          if (cfg.unit_lcd.i2c_port < 0) { cfg.unit_lcd.i2c_port = (int8_t)Ex_I2C.getPort(); }

          M5UnitLCD dsp(cfg.unit_lcd);
          int retry = 8;
          do {
            m5gfx::delay(32);
            if (dsp.init()) {
              addDisplay(dsp);
              port_a_used = true;
              break;
            }
          } while (--retry);
        }
#endif

// RCA is not available on ESP32S3
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
#if defined ( __M5GFX_M5MODULERCA__ ) || defined ( __M5GFX_M5UNITRCA__ )
        {
          bool unit_rca = cfg.external_display.unit_rca;
          (void)unit_rca;
#if defined ( __M5GFX_M5MODULERCA__ )
          if (cfg.external_display.module_rca)
          {
            if (board == board_t::board_M5Stack
            || board == board_t::board_M5StackCore2
            || board == board_t::board_M5Tough
            ) {
              // When ModuleRCA is used, UnitRCA is not used.
              unit_rca = false;
              M5ModuleRCA dsp(cfg.module_rca);
              if (dsp.init()) {
                addDisplay(dsp);
              }
            }
          }
#endif
#if defined ( __M5GFX_M5UNITRCA__ )
          if (unit_rca)
          {
            if ( board == board_t::board_M5Stack
              || board == board_t::board_M5StackCore2
              || board == board_t::board_M5Paper
              || board == board_t::board_M5Tough
              || board == board_t::board_M5Station
              || (!port_a_used && ( // ATOM does not allow video output via UnitRCA when PortA is used.
                   board == board_t::board_M5Atom
                || board == board_t::board_M5AtomPsram
                || board == board_t::board_M5AtomU
              )))
            {
              M5UnitRCA dsp(cfg.unit_rca);
              if (dsp.init()) {
                addDisplay(dsp);
              }
            }
          }
#endif
        }
#endif
#endif
      }

      if (_primaryDisplay.getBoard() != board_t::board_unknown)
      {
        _primaryDisplay.setBrightness(brightness);
      }

      update();
    }

  private:
    static constexpr std::size_t BTNPWR_MIN_UPDATE_MSEC = 4;

    std::uint32_t _updateMsec = 0;
    m5gfx::board_t _board = m5gfx::board_t::board_unknown;

    M5GFX _primaryDisplay;  // setPrimaryされたディスプレイのインスタンス
    std::vector<M5GFX> _displays; // 登録された全ディスプレイのインスタンス
    std::uint8_t _primary_display_index = -1;
    bool use_pmic_button = false;
    bool use_hat_spk = false;

    void _begin(const config_t& cfg);
    void _begin_spk(config_t& cfg);
    bool _begin_rtc_imu(const config_t& cfg);

    board_t _check_boardtype(board_t);
    void _setup_i2c(board_t);

    static bool _speaker_enabled_cb(void* args, bool enabled);
    static bool _microphone_enabled_cb(void* args, bool enabled);
  };
}

extern m5::M5Unified M5;

#endif