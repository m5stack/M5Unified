// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "M5Unified.hpp"

#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_sleep.h>
#include <esp_log.h>

/// global instance.
m5::M5Unified M5;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
 #define NON_BREAK ;[[fallthrough]];
#else
 #define NON_BREAK ;
#endif

namespace m5
{
  static constexpr int TFCARD_CS_PIN =  4;
  static constexpr int CoreInk_BUTTON_EXT_PIN =  5;
  static constexpr int CoreInk_BUTTON_PWR_PIN = 27;

  void M5Unified::begin(void)
  {
    auto brightness = Display.getBrightness();
    Display.startWrite(false);
    Display.setBrightness(0);
    Display.init_without_reset();
    _board = Display.getBoard();

    if (_board == board_t::board_unknown)
    {
#if defined ( ARDUINO_M5Stack_Core_ESP32 ) || defined ( ARDUINO_M5STACK_FIRE )

      _board = board_t::board_M5Stack;

#elif defined ( ARDUINO_M5STACK_Core2 )

      _board = board_t::board_M5StackCore2;

#elif defined ( ARDUINO_M5Stick_C )

      _board = board_t::board_M5StickC;

#elif defined ( ARDUINO_M5Stick_C_Plus )

      _board = board_t::board_M5StickCPlus;

#elif defined ( ARDUINO_M5Stack_CoreInk )

      _board = board_t::board_M5StackCoreInk;

#elif defined ( ARDUINO_M5STACK_Paper )

      _board = board_t::board_M5Paper;

#elif defined ( ARDUINO_M5STACK_TOUGH )

      _board = board_t::board_M5Tough;

#elif defined ( ARDUINO_M5Stack_ATOM )

      _board = board_t::board_M5ATOM;

#elif defined ( ARDUINO_M5Stack_Timer_CAM )

      _board = board_t::board_M5TimerCam;

#endif
    }

    { /// setup Internal I2C
      i2c_port_t in_port = I2C_NUM_1;
      int in_sda = 21;
      int in_scl = 22;
      switch (_board)
      {
      case board_t::board_M5ATOM:  // ATOM
        in_sda = 25;
        in_scl = 21;
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

    auto wakeup_cause = esp_sleep_get_wakeup_cause();
    Rtc.begin();
    if (!Rtc.getIRQstatus()
      && (wakeup_cause != ESP_SLEEP_WAKEUP_TIMER)
      && (wakeup_cause != ESP_SLEEP_WAKEUP_EXT0))
    { /// Does not clear the screen when waking up by timer or sleep release.
      Display.clear();
    }
    Rtc.disableIRQ();

    { /// setup External I2C
      i2c_port_t ex_port = I2C_NUM_0;
      int ex_sda = 32;
      int ex_scl = 33;
      switch (_board)
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

      default:
        break;
      }
      Ex_I2C.setPort(ex_port, ex_sda, ex_scl);
    }

    /// setup power management ic
    Power.begin();

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
      m5gfx::pinMode(39, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
      adc_power_acquire(); /// for GPIO 36,39 Chattering prevention.
      break;

    default:
      break;
    }

    Display.endWrite();
    Display.setBrightness(brightness);

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
      btns[0] = ! (raw_gpio32_40 & (1<<7)); // gpio39 A
      btns[1] = ! (raw_gpio32_40 & (1<<6)); // gpio38 B
      btns[2] = ! (raw_gpio32_40 & (1<<5)); // gpio37 C
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
