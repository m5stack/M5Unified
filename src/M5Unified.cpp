// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "M5Unified.hpp"

#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <soc/efuse_reg.h>
#include <esp_efuse.h>

/// global instance.
m5::M5Unified M5;

#if __has_include (<esp_idf_version.h>)
 #include <esp_idf_version.h>
 #if ESP_IDF_VERSION_MAJOR >= 4
  #define NON_BREAK ;[[fallthrough]];
 #endif
#endif

#ifndef NON_BREAK
#define NON_BREAK ;
#endif

namespace m5
{
  static constexpr int TFCARD_CS_PIN =  4;
  static constexpr int CoreInk_BUTTON_EXT_PIN =  5;
  static constexpr int CoreInk_BUTTON_PWR_PIN = 27;

  board_t M5Unified::_check_boardtype(board_t board)
  {
    if (board == board_t::board_unknown)
    {
      switch (esp_efuse_get_pkg_ver())
      {
      case EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ6:
        board = board_t::board_M5TimerCam;
        break;

      case EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4:
      case 6: // EFUSE_RD_CHIP_VER_PKG_ESP32PICOV3_02: // ATOM PSRAM
        board = board_t::board_M5ATOM;
        break;

      default:

#if defined ( ARDUINO_M5Stack_Core_ESP32 ) || defined ( ARDUINO_M5STACK_FIRE )

        board = board_t::board_M5Stack;

#elif defined ( ARDUINO_M5STACK_Core2 )

        board = board_t::board_M5StackCore2;

#elif defined ( ARDUINO_M5Stick_C )

        board = board_t::board_M5StickC;

#elif defined ( ARDUINO_M5Stick_C_Plus )

        board = board_t::board_M5StickCPlus;

#elif defined ( ARDUINO_M5Stack_CoreInk )

        board = board_t::board_M5StackCoreInk;

#elif defined ( ARDUINO_M5STACK_Paper )

        board = board_t::board_M5Paper;

#elif defined ( ARDUINO_M5STACK_TOUGH )

        board = board_t::board_M5Tough;

#elif defined ( ARDUINO_M5Stack_ATOM )

        board = board_t::board_M5ATOM;

#elif defined ( ARDUINO_M5Stack_Timer_CAM )

        board = board_t::board_M5TimerCam;

#endif
        break;
      }
    }

    { /// setup Internal I2C
      i2c_port_t in_port = I2C_NUM_1;
      int in_sda = 21;
      int in_scl = 22;
      switch (board)
      {
      case board_t::board_M5ATOM:  // ATOM
        in_sda = 25;
        in_scl = 21;
        break;

      case board_t::board_M5TimerCam:
        in_sda = 12;
        in_scl = 14;
        break;

      case board_t::board_M5Stack:
        // M5Stack basic/Fire/GO の内部I2CはGROVEポートと共通のため、I2C_NUM_0を用いる。;
        in_port = I2C_NUM_0;
        break;

      default:
        break;
      }
      In_I2C.begin(in_port, in_sda, in_scl);
    }

    { /// setup External I2C
      i2c_port_t ex_port = I2C_NUM_0;
      int ex_sda = 32;
      int ex_scl = 33;
      switch (board)
      {
      case board_t::board_M5Stack:
        ex_sda = 21;
        ex_scl = 22;
        break;

      case board_t::board_M5Paper:
        ex_sda = 25;
        ex_scl = 32;
        break;

      case board_t::board_M5ATOM:  // ATOM
        ex_sda = 26;
        ex_scl = 32;
        break;

      case board_t::board_M5TimerCam:
        ex_sda =  4;
        ex_scl = 13;
        break;

      default:
        break;
      }
      Ex_I2C.setPort(ex_port, ex_sda, ex_scl);
    }
    return board;
  }

  void M5Unified::_begin(void)
  {
    /// setup power management ic
    Power.begin();

    auto wakeup_cause = esp_sleep_get_wakeup_cause();
    Rtc.begin();
    if (!Rtc.getIRQstatus()
      && (wakeup_cause != ESP_SLEEP_WAKEUP_TIMER)
      && (wakeup_cause != ESP_SLEEP_WAKEUP_EXT0))
    { /// Does not clear the screen when waking up by timer or sleep release.
      Display.clear();
    }
    Rtc.disableIRQ();

    switch (_board) /// setup Hardware Buttons
    {
    case board_t::board_M5StackCoreInk:
      m5gfx::pinMode(CoreInk_BUTTON_EXT_PIN, m5gfx::pin_mode_t::input); // TopButton
      m5gfx::pinMode(CoreInk_BUTTON_PWR_PIN, m5gfx::pin_mode_t::input); // PowerButton
      NON_BREAK; /// don't break;

    case board_t::board_M5Paper:
    case board_t::board_M5Station:
    case board_t::board_M5Stack:
      m5gfx::pinMode(38, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      m5gfx::pinMode(37, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5ATOM:
      m5gfx::pinMode(39, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
 /// for GPIO 36,39 Chattering prevention.
#if defined (ESP_IDF_VERSION_VAL)
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(3, 3, 5)
      adc_power_acquire();
  #else
      adc_power_on();
  #endif
#else
      adc_power_on();
#endif
      break;

    default:
      break;
    }

    if (nullptr != Display.touch())
    {
      Touch.begin(&Display);
    }
  }


  void M5Unified::update( void )
  {
    bool btns[5] = { false, false, false, false, false };
    auto ms = m5gfx::millis();
    if (Touch.isEnabled())
    {
      Touch.update(ms);
      switch (_board)
      {
      case board_t::board_M5StackCore2:
        {
          auto count = Touch.getCount();
          for (size_t i = 0; i < count; ++i)
          {
            auto det = Touch.getDetail(i);
            if ((det.state & touch_state_t::mask_touch)
             && !(det.state & touch_state_t::mask_moving))
            {
              auto raw = Touch.getTouchPointRaw(i);
              if (raw.y > 240)
              {
                size_t idx = raw.x / 110;
                if (raw.x - (idx * 110) < 100)
                {
                  btns[idx] = true;
                }
              }
            }
          }
        }
        break;

      default:
        break;
      }
    }

    uint32_t raw_gpio0_31 = GPIO.in;
    uint32_t raw_gpio32_40 = GPIO.in1.data;
    switch (_board)
    {
    case board_t::board_M5StackCoreInk:
      btns[4] = ! (raw_gpio0_31 & (1<<CoreInk_BUTTON_EXT_PIN));
      btns[3] = ! (raw_gpio0_31 & (1<<CoreInk_BUTTON_PWR_PIN));
      NON_BREAK; /// don't break;

    case board_t::board_M5Paper:
    case board_t::board_M5Station:
      btns[0] = ! (raw_gpio32_40 & (1<<5)); // gpio37 A
      btns[1] = ! (raw_gpio32_40 & (1<<6)); // gpio38 B
      btns[2] = ! (raw_gpio32_40 & (1<<7)); // gpio39 C
      break;

    case board_t::board_M5Stack:
      btns[2] = ! (raw_gpio32_40 & (1<<5)); // gpio37 C
      btns[1] = ! (raw_gpio32_40 & (1<<6)); // gpio38 B
      NON_BREAK; /// don't break;

    case board_t::board_M5ATOM:
    case board_t::board_M5AtomDisplay:
      btns[0] = ! (raw_gpio32_40 & (1<<7)); // gpio39 A
      break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      btns[0] = ! (raw_gpio32_40 & (1<<5)); // gpio37
      btns[1] = ! (raw_gpio32_40 & (1<<7)); // gpio39
      break;

    default:
      break;
    }

    BtnA  .setRawState(ms, btns[0]);
    BtnB  .setRawState(ms, btns[1]);
    BtnC  .setRawState(ms, btns[2]);
    BtnEXT.setRawState(ms, btns[4]);
    if (Power.Axp192.isEnabled())
    {
      auto tmp = Power.Axp192.getPekPress();
      if (tmp == 1) { tmp = 2; }
      else if (tmp == 2) { tmp = 1; }
      BtnPWR.setState(ms, tmp);
    }
    else
    {
      BtnPWR.setRawState(ms, btns[3]);
    }
  }
}
