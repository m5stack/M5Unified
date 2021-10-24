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

#include "utility/BM8563_Class.hpp"
#include "utility/AXP192_Class.hpp"
#include "utility/IP5306_Class.hpp"
#include "utility/IMU_Class.hpp"
#include "utility/Button_Class.hpp"
#include "utility/Power_Class.hpp"
#include "utility/Touch_Class.hpp"

namespace m5
{
  using board_t = m5gfx::board_t;
  using touch_point_t = m5gfx::touch_point_t;
  using touch_detail_t = Touch_Class::touch_detail_t;

  class M5Unified
  {
  public:
    /// get the board type of the runtime environment.
    /// @return board type
    board_t getBoard(void) const { return _board; }

    /// Perform initialization process at startup.
    void begin(void);

    /// To call this function in a loop function.
    void update(void);

    M5GFX Display;
    M5GFX &Lcd = Display;

    IMU_Class Imu;
    Power_Class Power;
    BM8563_Class Rtc;
    Touch_Class Touch;

/*
  /// List of available buttons:
  M5Stack BASIC/GRAY/GO/FIRE:  BtnA,BtnB,BtnC
  M5Stack Core2:               BtnA,BtnB,BtnC,BtnPWR
  M5Stick C/CPlus:             BtnA,BtnB,     BtnPWR
  M5Stick CoreInk:             BtnA,BtnB,BtnC,BtnPWR,BtnEXT
  M5Paper:                     BtnA,BtnB,BtnC
  M5Station:                   BtnA,BtnB,BtnC
  M5Tough:                                    BtnPWR
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
  };
}

extern m5::M5Unified M5;

#endif