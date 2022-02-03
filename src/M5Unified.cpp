// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "M5Unified.hpp"

#if __has_include (<esp_idf_version.h>)
 #include <esp_idf_version.h>
 #if ESP_IDF_VERSION_MAJOR >= 4
  /// [[fallthrough]];
  #define NON_BREAK ;[[fallthrough]];
 #endif
#endif

/// [[fallthrough]];
#ifndef NON_BREAK
#define NON_BREAK ;
#endif

/// global instance.
m5::M5Unified M5;

#include <soc/efuse_reg.h>

#if __has_include (<driver/adc.h>)
 #include <driver/adc.h>
#endif

void __attribute((weak)) adc_power_acquire(void)
{
#if defined (ESP_IDF_VERSION_VAL)
 #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(3, 3, 4)
  adc_power_on();
 #endif
#else
 adc_power_on();
#endif
}

namespace m5
{
  bool M5Unified::_sound_set_mode_cb(void* args, m5::sound_mode_t mode)
  {
    auto self = (M5Unified*)args;

    switch (self->getBoard())
    {
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
      self->Power.Axp192.setGPIO2(mode == sound_mode_t::sound_output);
      break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      self->Power.Axp192.setLDO0(mode == sound_mode_t::sound_input ? 2800 : 0);
      NON_BREAK;

    case board_t::board_M5StackCoreInk:
      /// for SPK HAT
      if ((self->_cfg.external_spk_detail.enabled) && !self->_cfg.external_spk_detail.omit_spk_hat)
      {
        gpio_num_t pin_en = self->_board == board_t::board_M5StackCoreInk ? GPIO_NUM_25 : GPIO_NUM_0;
        if (mode == sound_mode_t::sound_output)
        {
          m5gfx::pinMode(pin_en, m5gfx::pin_mode_t::output);
          m5gfx::gpio_hi(pin_en);
        }
        else
        { m5gfx::gpio_lo(pin_en); }
      }
      NON_BREAK;

      break;

#endif
    default:
      break;
    }
    return true;
  }



#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
  static constexpr gpio_num_t TFCARD_CS_PIN          = GPIO_NUM_4;
  static constexpr gpio_num_t CoreInk_BUTTON_EXT_PIN = GPIO_NUM_5;
  static constexpr gpio_num_t CoreInk_BUTTON_PWR_PIN = GPIO_NUM_27;
#endif
  board_t M5Unified::_check_boardtype(board_t board)
  {
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (board == board_t::board_unknown)
    {
      switch (m5gfx::get_pkg_ver())
      {
      case EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ6:
        board = board_t::board_M5TimerCam;
        break;

      case EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4:
        m5gfx::pinMode(GPIO_NUM_2, m5gfx::pin_mode_t::input_pullup);
        m5gfx::pinMode(GPIO_NUM_34, m5gfx::pin_mode_t::input);
        board = m5gfx::gpio_in(GPIO_NUM_2)
              ? (m5gfx::gpio_in(GPIO_NUM_34) ? board_t::board_M5Atom : board_t::board_M5AtomU)
              : board_t::board_M5StampPico;
        m5gfx::pinMode(GPIO_NUM_2, m5gfx::pin_mode_t::input_pulldown);
        break;

      case 6: // EFUSE_RD_CHIP_VER_PKG_ESP32PICOV3_02: // ATOM PSRAM
        board = board_t::board_M5AtomPsram;
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

        board = board_t::board_M5Atom;

#elif defined ( ARDUINO_M5Stack_Timer_CAM )

        board = board_t::board_M5TimerCam;

#endif
        break;
      }
    }

    { /// setup Internal I2C
      i2c_port_t in_port = I2C_NUM_1;
      gpio_num_t in_sda = GPIO_NUM_21;
      gpio_num_t in_scl = GPIO_NUM_22;
      switch (board)
      {
      case board_t::board_M5Atom:  // ATOM
      case board_t::board_M5AtomU:
      case board_t::board_M5AtomPsram:
        in_sda = GPIO_NUM_25;
        in_scl = GPIO_NUM_21;
        break;

      case board_t::board_M5TimerCam:
        in_sda = GPIO_NUM_12;
        in_scl = GPIO_NUM_14;
        break;

      case board_t::board_M5Stack:
        // M5Stack Basic/Fire/GO の内部I2CはPortAと共通のため、I2C_NUM_0を用いる。;
        in_port = I2C_NUM_0;
        break;

      default:
        break;
      }
      In_I2C.begin(in_port, in_sda, in_scl);
    }

    { /// setup External I2C
      i2c_port_t ex_port = I2C_NUM_0;
      gpio_num_t ex_sda = GPIO_NUM_32;
      gpio_num_t ex_scl = GPIO_NUM_33;
      switch (board)
      {
      case board_t::board_M5Stack:
        ex_sda = GPIO_NUM_21;
        ex_scl = GPIO_NUM_22;
        break;

      case board_t::board_M5Paper:
        ex_sda = GPIO_NUM_25;
        ex_scl = GPIO_NUM_32;
        break;

      case board_t::board_M5Atom:
      case board_t::board_M5AtomU:
      case board_t::board_M5AtomPsram:
        ex_sda = GPIO_NUM_26;
        ex_scl = GPIO_NUM_32;
        break;

      case board_t::board_M5TimerCam:
        ex_sda = GPIO_NUM_4;
        ex_scl = GPIO_NUM_13;
        break;

      default:
        break;
      }
      Ex_I2C.setPort(ex_port, ex_sda, ex_scl);
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C3)

    if (board == board_t::board_unknown)
    {
      m5gfx::pinMode(GPIO_NUM_20, m5gfx::pin_mode_t::input_pulldown);
      board = m5gfx::gpio_in(GPIO_NUM_20)
            ? board_t::board_M5StampC3
            : board_t::board_M5StampC3U
            ;
    }

    {
      i2c_port_t in_port = I2C_NUM_0;
      gpio_num_t in_sda = GPIO_NUM_1;
      gpio_num_t in_scl = GPIO_NUM_0;
      In_I2C.begin(in_port, in_sda, in_scl);
    }

    { /// setup External I2C
      i2c_port_t ex_port = I2C_NUM_0;
      gpio_num_t ex_sda = GPIO_NUM_1;
      gpio_num_t ex_scl = GPIO_NUM_0;
      Ex_I2C.setPort(ex_port, ex_sda, ex_scl);
    }
#endif

    return board;
  }

  void M5Unified::_begin(void)
  {
    /// setup power management ic
    Power.begin();
    Power.setExtPower(_cfg.output_power);
    if (_cfg.led_brightness)
    {
      M5.Power.setLed(_cfg.led_brightness);
    }

    if (_cfg.clear_display)
    {
      Display.clear();
    }
    if (nullptr != Display.touch())
    {
      Touch.begin(&Display);
    }

#if defined ( ARDUINO )

    if (_cfg.serial_baudrate)
    {
      Serial.begin(_cfg.serial_baudrate);
    }

#endif

    if (_cfg.internal_spk || _cfg.external_spk_detail.enabled || _cfg.internal_mic)
    {
      auto sound_cfg = Sound.config();

      if (_cfg.internal_mic)
      {
        sound_cfg.mic_gain = 10;
        sound_cfg.mic_over_sampling = 2;
        switch (_board)
        {
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
        case board_t::board_M5Stack:
          if (_cfg.internal_mic)
          {
            sound_cfg.pin_data_in = 34;  // M5GO bottom MIC
            sound_cfg.mic_adc = true;
            sound_cfg.mic_offset = 192;
            sound_cfg.mic_over_sampling = 4;
          }
          break;

        case board_t::board_M5StickC:
        case board_t::board_M5StickCPlus:
        case board_t::board_M5Tough:
        case board_t::board_M5StackCore2:
          if (_cfg.internal_mic)
          { /// builtin PDM mic
            sound_cfg.pin_data_in = 34;
            sound_cfg.pin_lrck = 0;
          }
          break;

        case board_t::board_M5AtomU:
          { /// ATOM U builtin PDM mic
            sound_cfg.pin_lrck = 5;
            sound_cfg.pin_data_in = 19;
            sound_cfg.mic_offset = - 768;
          }
          break;

        case board_t::board_M5Atom:
          { /// ATOM ECHO builtin PDM mic
            sound_cfg.pin_lrck = 33;
            sound_cfg.pin_data_in = 23;
          }
          break;
#endif
        default:
          break;
        }
      }

      if (_cfg.internal_spk || _cfg.external_spk_detail.enabled)
      {
        sound_cfg.spk_gain = 127;
        switch (_board)
        {
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
        case board_t::board_M5Stack:
          if (_cfg.internal_spk)
          {
            m5gfx::gpio_lo(GPIO_NUM_25);
            m5gfx::pinMode(GPIO_NUM_25, m5gfx::pin_mode_t::output);
            sound_cfg.spk_dac = true;
            sound_cfg.pin_data_out = 25;
          }
          break;

        case board_t::board_M5StackCoreInk:
        case board_t::board_M5StickCPlus:
          if (_cfg.internal_spk)
          {
            sound_cfg.spk_buzzer = true;
            sound_cfg.pin_data_out = 2;
          }
          NON_BREAK;
        case board_t::board_M5StickC:
          if (_cfg.external_spk_detail.enabled && !_cfg.external_spk_detail.omit_spk_hat)
          { /// for SPK HAT
            gpio_num_t pin_en = _board == board_t::board_M5StackCoreInk ? GPIO_NUM_25 : GPIO_NUM_0;
            m5gfx::gpio_lo(pin_en);
            m5gfx::pinMode(pin_en, m5gfx::pin_mode_t::output);
            m5gfx::gpio_lo(GPIO_NUM_26);
            m5gfx::pinMode(GPIO_NUM_26, m5gfx::pin_mode_t::output);
            sound_cfg.pin_data_out = 26;
            sound_cfg.spk_dac = true;
            sound_cfg.spk_buzzer = false;
          }
          sound_cfg.spk_gain = 255;
          break;

        case board_t::board_M5Tough:
          sound_cfg.spk_gain = 191;
          NON_BREAK;
        case board_t::board_M5StackCore2:
          if (_cfg.internal_spk)
          {
            sound_cfg.pin_bck = 12;
            sound_cfg.pin_lrck = 0;
            sound_cfg.pin_data_out = 2;
          }
          break;

        case board_t::board_M5Atom:
          if (_cfg.internal_spk)
          {
            sound_cfg.pin_bck = 19;
            sound_cfg.pin_lrck = 33;
            sound_cfg.pin_data_out = 22;
            sound_cfg.spk_gain = 63;
          }
          NON_BREAK;
        case board_t::board_M5AtomPsram:
          if (_cfg.external_spk_detail.enabled && !_cfg.external_spk_detail.omit_atomic_spk)
          { /// for ATOMIC SPK
            /// 19,23,33 pulldown read check ( all high = ATOMIC_SPK ? )
            gpio_num_t pin = (_board == board_t::board_M5AtomPsram) ? GPIO_NUM_5 : GPIO_NUM_23;
            m5gfx::pinMode(GPIO_NUM_19, m5gfx::pin_mode_t::input_pulldown);
            m5gfx::pinMode(GPIO_NUM_33, m5gfx::pin_mode_t::input_pulldown);
            m5gfx::pinMode(pin        , m5gfx::pin_mode_t::input_pulldown);
            if (m5gfx::gpio_in(GPIO_NUM_19)
             && m5gfx::gpio_in(GPIO_NUM_33)
             && m5gfx::gpio_in(pin        ))
            {
              _cfg.internal_imu = false; /// avoid conflict with i2c
              _cfg.internal_rtc = false; /// avoid conflict with i2c
              sound_cfg.pin_bck = 22;
              sound_cfg.pin_lrck = 21;
              sound_cfg.pin_data_out = 25;
              sound_cfg.spk_gain = 63;
            }
          }
          break;
#endif
        default:
          break;
        }
      }

      Sound.setCallback(this, _sound_set_mode_cb);
      Sound.config(sound_cfg);
    }

    switch (_board) /// setup Hardware Buttons
    {
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case board_t::board_M5StackCoreInk:
      m5gfx::pinMode(CoreInk_BUTTON_EXT_PIN, m5gfx::pin_mode_t::input); // TopButton
      m5gfx::pinMode(CoreInk_BUTTON_PWR_PIN, m5gfx::pin_mode_t::input); // PowerButton
      NON_BREAK; /// don't break;

    case board_t::board_M5Paper:
    case board_t::board_M5Station:
    case board_t::board_M5Stack:
      m5gfx::pinMode(GPIO_NUM_38, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      m5gfx::pinMode(GPIO_NUM_37, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5Atom:
    case board_t::board_M5AtomPsram:
    case board_t::board_M5AtomU:
    case board_t::board_M5StampPico:
      m5gfx::pinMode(GPIO_NUM_39, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
 /// for GPIO 36,39 Chattering prevention.
      adc_power_acquire();
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32C3)

    case board_t::board_M5StampC3:
      m5gfx::pinMode(GPIO_NUM_3, m5gfx::pin_mode_t::input_pullup);
      break;

    case board_t::board_M5StampC3U:
      m5gfx::pinMode(GPIO_NUM_9, m5gfx::pin_mode_t::input_pullup);
      break;

#endif

    default:
      break;
    }

    if (_cfg.external_rtc || _cfg.external_imu)
    {
      M5.Ex_I2C.begin();
    }

    if (_cfg.internal_rtc)
    {
      M5.Rtc.begin();
    }
    if (!M5.Rtc.isEnabled() && _cfg.external_rtc)
    {
      M5.Rtc.begin(&M5.Ex_I2C);
    }

    M5.Rtc.setSystemTimeFromRtc();

    if (_cfg.internal_imu)
    {
      if (M5.Imu.begin())
      {
        if (M5.getBoard() == m5::board_t::board_M5Atom)
        { // ATOM Matrix's IMU is oriented differently, so change the setting.
          M5.Imu.setRotation(2);
        }
      }
    }
    if (!M5.Imu.isEnabled() && _cfg.external_imu)
    {
      M5.Imu.begin(&M5.Ex_I2C);
    }
  }


  void M5Unified::update( void )
  {
    auto ms = m5gfx::millis();

    if (Touch.isEnabled())
    {
      Touch.update(ms);
    }

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    uint_fast8_t raw_gpio37_40 = ~GPIO.in1.data >> 5;
    uint_fast8_t btn_bits = 0;
    switch (_board)
    {
    case board_t::board_M5StackCore2:
      {
        int i = Touch.getCount();
        while (--i >= 0)
        {
          auto det = Touch.getDetail(i);
          if ((det.state & (touch_state_t::mask_touch | touch_state_t::mask_moving)) == touch_state_t::mask_touch)
          {
            auto raw = Touch.getTouchPointRaw(i);
            if (raw.y > 240)
            {
              int x = raw.x;
              size_t idx = x / 110;
              if (x - (idx * 110) < 100)
              {
                btn_bits = 1 << idx;
              }
            }
          }
        }
      }
      break;

    case board_t::board_M5StackCoreInk:
      {
        uint32_t raw_gpio0_31 = ~GPIO.in;
        BtnEXT.setRawState(ms, (raw_gpio0_31 & (1 << CoreInk_BUTTON_EXT_PIN)));
        BtnPWR.setRawState(ms, (raw_gpio0_31 & (1 << CoreInk_BUTTON_PWR_PIN)));
      }
      NON_BREAK; /// don't break;

    case board_t::board_M5Paper:
    case board_t::board_M5Station:
      btn_bits = raw_gpio37_40 & 0x07; // gpio37 A / gpio38 B / gpio39 C
      break;

    case board_t::board_M5Stack:
      btn_bits = ( raw_gpio37_40 & 0x02)        // gpio38 B
               + ((raw_gpio37_40 & 0x01) << 2); // gpio37 C
      NON_BREAK; /// don't break;

    case board_t::board_M5Atom:
    case board_t::board_M5AtomPsram:
    case board_t::board_M5AtomU:
    case board_t::board_M5StampPico:
      btn_bits += (raw_gpio37_40 & 0x04) >> 2; // gpio39 A
      break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      btn_bits = ( raw_gpio37_40 & 0x01)        // gpio37 A
               + ((raw_gpio37_40 & 0x04) >> 1); // gpio39 B
      break;

    default:
      break;
    }

    BtnA.setRawState(ms, btn_bits & 1);
    BtnB.setRawState(ms, btn_bits & 2);
    BtnC.setRawState(ms, btn_bits & 4);
    if (Power.Axp192.isEnabled())
    {
      auto tmp = Power.Axp192.getPekPress();
      if (tmp == 1) { tmp = 2; }
      else if (tmp == 2) { tmp = 1; }
      BtnPWR.setState(ms, tmp);
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C3)

    switch (_board)
    {
    case board_t::board_M5StampC3:
      BtnA.setRawState(ms, !m5gfx::gpio_in(GPIO_NUM_3));
      break;

    case board_t::board_M5StampC3U:
      BtnA.setRawState(ms, !m5gfx::gpio_in(GPIO_NUM_9));
      break;

    default:
      break;
    }

#endif
  }
}
