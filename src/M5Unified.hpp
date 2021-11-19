// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5UNIFIED_HPP__
#define __M5UNIFIED_HPP__

#if defined (ARDUINO)
 #if __has_include(<SD.h>)
  #include <SD.h>
 #endif
 #if __has_include(<SPIFFS.h>)
  #include <SPIFFS.h>
 #endif
 #if __has_include(<LittleFS.h>)
  #include <LittleFS.h>
 #endif
 #if __has_include(<LITTLEFS.h>)
  #include <LITTLEFS.h>
 #endif
 #if __has_include(<PSRamFS.h>)
  #include <PSRamFS.h>
 #endif
#endif

#include <M5GFX.h>

#include "utility/RTC8563_Class.hpp"
#include "utility/AXP192_Class.hpp"
#include "utility/IP5306_Class.hpp"
#include "utility/IMU_Class.hpp"
#include "utility/Button_Class.hpp"
#include "utility/Power_Class.hpp"
#include "utility/Touch_Class.hpp"

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

      /// use "Serial" begin.
      uint32_t serial_baudrate = 115200;

#endif

      /// Clear the screen.
      bool clear_display = true;

      /// use external port 5V output.
      bool output_power  = true;

      /// use internal IMU.
      bool internal_imu  = true;

      /// use internal RTC.
      bool internal_rtc  = true;

      /// use Unit Accel & Gyro.
      bool external_imu  = false;

      /// use Unit RTC.
      bool external_rtc  = false;

      /// system LED brightness (0=off / 255=max) (※ not NeoPixel)
      uint8_t led_brightness = 0;
    };

    config_t config(void) const { return config_t(); }

    /// get the board type of the runtime environment.
    /// @return board type
    board_t getBoard(void) const { return _board; }

    /// Perform initialization process at startup.
    void begin(void)
    {
      begin(config_t{});
    }
    void begin(const config_t& cfg)
    {
      auto brightness = Display.getBrightness();
      Display.setBrightness(0);
      bool res = Display.init_without_reset();
      _board = _check_boardtype(Display.getBoard());
      if (!res)
      {
        Display._set_board(_switch_display());
      }
      _begin(cfg);
      Display.setBrightness(brightness);
    }

    /// To call this function in a loop function.
    void update(void);


    M5GFX Display;
    M5GFX &Lcd = Display;

    IMU_Class Imu;
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

  private:
    m5gfx::board_t _board = m5gfx::board_t::board_unknown;

    void _begin(const config_t& cfg);
    board_t _check_boardtype(board_t);

    std::unique_ptr<m5gfx::LGFX_Device> _ex_display;
    board_t _switch_display(void)
    {
#if defined ( __M5GFX_M5ATOMDISPLAY__ )
      if (_board == board_t::board_M5ATOM)
      {
#ifndef M5ATOMDISPLAY_WIDTH
#define M5ATOMDISPLAY_WIDTH 1280
#endif
#ifndef M5ATOMDISPLAY_HEIGHT
#define M5ATOMDISPLAY_HEIGHT 720
#endif
#ifndef M5ATOMDISPLAY_RATE
#define M5ATOMDISPLAY_RATE 60
#endif
        auto dsp = new M5AtomDisplay(M5ATOMDISPLAY_WIDTH, M5ATOMDISPLAY_HEIGHT, M5ATOMDISPLAY_RATE);
        _ex_display.reset(dsp);
        if (Display._init_with_panel(dsp->getPanel()))
        {
          return dsp->getBoard();
        }
      }
#endif
#if defined ( __M5GFX_M5UNITLCD__ ) || defined ( __M5GFX_M5UNITOLED__ )
      Power.setExtPower(true);
#endif
#if defined ( __M5GFX_M5UNITLCD__ )
      {
        auto dsp = new M5UnitLCD(Ex_I2C.getSDA(), Ex_I2C.getSCL(), 400000, Ex_I2C.getPort());
        _ex_display.reset(dsp);
        if (Display._init_with_panel(dsp->getPanel()))
        {
          return dsp->getBoard();
        }
      }
#endif
#if defined ( __M5GFX_M5UNITOLED__ )
      {
        auto dsp = new M5UnitOLED(Ex_I2C.getSDA(), Ex_I2C.getSCL(), 400000, Ex_I2C.getPort());
        _ex_display.reset(dsp);
        if (Display._init_with_panel(dsp->getPanel()))
        {
          return dsp->getBoard();
        }
      }
#endif
      return Display.getBoard();
    }
  };
}

extern m5::M5Unified M5;

#endif