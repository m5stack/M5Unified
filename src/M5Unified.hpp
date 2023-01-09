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
      bool internal_mic = true;

      /// use the speaker.
      bool internal_spk = true;

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
          uint8_t reserve : 5;
        } external_spk_detail;
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
      auto brightness = Display.getBrightness();
      Display.setBrightness(0);
      bool res = Display.init_without_reset();
      _board = _check_boardtype(Display.getBoard());
      if (!res)
      {
        ((M5GFX_*)&Display)->setBoard(_switch_display());
      }
      _begin();
      Display.setBrightness(brightness);
    }

    /// To call this function in a loop function.
    void update(void);

    /// milli seconds at the time the update was called
    std::uint32_t getUpdateMsec(void) const { return _updateMsec; }

    M5GFX Display;
    M5GFX &Lcd = Display;

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

  private:
    static constexpr std::size_t BTNPWR_MIN_UPDATE_MSEC = 4;

    std::uint32_t _updateMsec = 0;
    config_t _cfg;
    m5gfx::board_t _board = m5gfx::board_t::board_unknown;

    void _begin(void);
    board_t _check_boardtype(board_t);

    static bool _speaker_enabled_cb(void* args, bool enabled);
    static bool _microphone_enabled_cb(void* args, bool enabled);
    // static bool _sound_set_mode_cb(void* args, m5::sound_mode_t mode);

    std::unique_ptr<m5gfx::LGFX_Device> _ex_display;
    board_t _switch_display(void)
    {
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
#if defined ( __M5GFX_M5ATOMDISPLAY__ )
      if (_board == board_t::board_M5ATOM)
      {
ESP_LOGD("M5Unified","check AtomDisplay");
        auto dsp = new M5AtomDisplay();
        _ex_display.reset(dsp);
        // if (((M5GFX_*)&Display)->init_with_panel(dsp->getPanel()))
        if (dsp->init())
        {
          Display.setPanel(dsp->getPanel());
          (lgfx::LGFX_Device)Display = *(lgfx::LGFX_Device*)dsp;
          Display.init();
ESP_LOGD("M5Unified","use AtomDisplay");
          return dsp->getBoard();
        }
      }
#endif
#endif

#if defined ( __M5GFX_M5UNITOLED__ )
      {
        auto dsp = new M5UnitOLED(Ex_I2C.getSDA(), Ex_I2C.getSCL(), 400000, Ex_I2C.getPort());
        _ex_display.reset(dsp);
        if (((M5GFX_*)&Display)->init_with_panel(dsp->getPanel()))
        {
          return dsp->getBoard();
        }
      }
#endif

#if defined ( __M5GFX_M5UNITLCD__ )
      { // The UnitLCD has a delay to wait for the time to start operation after power-on.
        m5gfx::delay(100);
        auto dsp = new M5UnitLCD(Ex_I2C.getSDA(), Ex_I2C.getSCL(), 400000, Ex_I2C.getPort());
        _ex_display.reset(dsp);
        if (((M5GFX_*)&Display)->init_with_panel(dsp->getPanel()))
        {
          return dsp->getBoard();
        }
      }
#endif
      _ex_display.reset(nullptr);
      ((M5GFX_*)&Display)->init_with_panel(nullptr);
      return Display.getBoard();
    }
public:
    struct M5GFX_ : public M5GFX
    {
      void setBoard(board_t board) { _board = board; }
      bool init_with_panel(lgfx::Panel_Device* panel)
      {
        setPanel(panel);
        return LGFX_Device::init_impl(true, true);
      }
    };
  };
}

extern m5::M5Unified M5;

#endif