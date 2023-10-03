// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "M5Unified.hpp"

#if !defined (M5UNIFIED_PC_BUILD)
#include <soc/efuse_reg.h>
#include <soc/gpio_periph.h>

#if __has_include (<driver/i2s.h>)
#include <driver/i2s.h>
#endif

#if __has_include (<esp_idf_version.h>)
 #include <esp_idf_version.h>
 #if ESP_IDF_VERSION_MAJOR >= 4
  /// [[fallthrough]];
  #define NON_BREAK ;[[fallthrough]];
 #endif

#endif
#endif

/// [[fallthrough]];
#ifndef NON_BREAK
#define NON_BREAK ;
#endif

/// global instance.
m5::M5Unified M5;

void __attribute((weak)) adc_power_acquire(void)
{
#if !defined (M5UNIFIED_PC_BUILD)
#if defined (ESP_IDF_VERSION_VAL)
 #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(3, 3, 4)
  adc_power_on();
 #endif
#else
 adc_power_on();
#endif
#endif
}

namespace m5
{
#if defined (CONFIG_IDF_TARGET_ESP32S3)
  static constexpr uint8_t aw88298_i2c_addr = 0x36;
  static constexpr uint8_t es7210_i2c_addr = 0x40;
  static constexpr uint8_t aw9523_i2c_addr = 0x58;
  static void aw88298_write_reg(uint8_t reg, uint16_t value)
  {
    value = __builtin_bswap16(value);
    M5.In_I2C.writeRegister(aw88298_i2c_addr, reg, (const uint8_t*)&value, 2, 400000);
  }

  static void es7210_write_reg(uint8_t reg, uint8_t value)
  {
    M5.In_I2C.writeRegister(es7210_i2c_addr, reg, &value, 1, 400000);
  }

#endif

  bool M5Unified::_speaker_enabled_cb(void* args, bool enabled)
  {
    auto self = (M5Unified*)args;

    switch (self->getBoard())
    {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StackCoreS3:
      {
        auto cfg = self->Speaker.config();
        if (cfg.pin_bck == GPIO_NUM_34 && enabled)
        {
          self->In_I2C.bitOn(aw9523_i2c_addr, 0x02, 0b00000100, 400000);
          /// サンプリングレートに応じてAW88298のレジスタの設定値を変える;
          static constexpr uint8_t rate_tbl[] = {4,5,6,8,10,11,15,20,22,44};
          size_t reg0x06_value = 0;
          size_t rate = (cfg.sample_rate + 1102) / 2205;
          while (rate > rate_tbl[reg0x06_value] && ++reg0x06_value < sizeof(rate_tbl)) {}

          reg0x06_value |= 0x14C0;  // I2SBCK=0 (BCK mode 16*2)
          aw88298_write_reg( 0x61, 0x0673 );  // boost mode disabled 
          aw88298_write_reg( 0x04, 0x4040 );  // I2SEN=1 AMPPD=0 PWDN=0
          aw88298_write_reg( 0x05, 0x0008 );  // RMSE=0 HAGCE=0 HDCCE=0 HMUTE=0
          aw88298_write_reg( 0x06, reg0x06_value );
          aw88298_write_reg( 0x0C, 0x0064 );  // volume setting (full volume)
        }
        else /// disableにする場合および内蔵スピーカ以外を操作対象とした場合、内蔵スピーカを停止する。
        {
          aw88298_write_reg( 0x04, 0x4000 );  // I2SEN=0 AMPPD=0 PWDN=0
          self->In_I2C.bitOff(aw9523_i2c_addr, 0x02, 0b00000100, 400000);
        }
      }
      break;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
      {
        auto spk_cfg = self->Speaker.config();
        if (spk_cfg.pin_bck      == GPIO_NUM_12
         && spk_cfg.pin_ws       == GPIO_NUM_0
         && spk_cfg.pin_data_out == GPIO_NUM_2
        ) {
          switch (self->Power.getType()) {
          case m5::Power_Class::pmic_axp192:
            self->Power.Axp192.setGPIO2(enabled);
            break;
          case m5::Power_Class::pmic_axp2101:
            self->Power.Axp2101.setALDO3(enabled * 3300);
            break;
          default:
            break;
          }
        }
      }
      break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
    case board_t::board_M5StickCPlus2:
    case board_t::board_M5StackCoreInk:
      /// for SPK HAT
      if (self->use_hat_spk)
      {
        gpio_num_t pin_en = self->_board == board_t::board_M5StackCoreInk ? GPIO_NUM_25 : GPIO_NUM_0;
        if (enabled)
        {
          m5gfx::pinMode(pin_en, m5gfx::pin_mode_t::output);
          m5gfx::gpio_hi(pin_en);
        }
        else
        { m5gfx::gpio_lo(pin_en); }
      }
      break;

#endif
    default:
      break;
    }
    return true;
  }

  bool M5Unified::_microphone_enabled_cb(void* args, bool enabled)
  {
    auto self = (M5Unified*)args;

    switch (self->getBoard())
    {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StackCoreS3:
      {
        auto cfg = self->Mic.config();
        if (cfg.pin_bck == GPIO_NUM_34)
        {
          es7210_write_reg(0x00, 0xFF); // RESET_CTL
          struct __attribute__((packed)) reg_data_t
          {
            uint8_t reg;
            uint8_t value;
          };
          if (enabled)
          {
            static constexpr reg_data_t data[] =
            {
              { 0x00, 0x41 }, // RESET_CTL
              { 0x01, 0x1f }, // CLK_ON_OFF
              { 0x06, 0x00 }, // DIGITAL_PDN
              { 0x07, 0x20 }, // ADC_OSR
              { 0x08, 0x10 }, // MODE_CFG
              { 0x09, 0x30 }, // TCT0_CHPINI
              { 0x0A, 0x30 }, // TCT1_CHPINI
              { 0x20, 0x0a }, // ADC34_HPF2
              { 0x21, 0x2a }, // ADC34_HPF1
              { 0x22, 0x0a }, // ADC12_HPF2
              { 0x23, 0x2a }, // ADC12_HPF1
              { 0x02, 0xC1 },
              { 0x04, 0x01 },
              { 0x05, 0x00 },
              { 0x11, 0x60 },
              { 0x40, 0x42 }, // ANALOG_SYS
              { 0x41, 0x70 }, // MICBIAS12
              { 0x42, 0x70 }, // MICBIAS34
              { 0x43, 0x1B }, // MIC1_GAIN
              { 0x44, 0x1B }, // MIC2_GAIN
              { 0x45, 0x00 }, // MIC3_GAIN
              { 0x46, 0x00 }, // MIC4_GAIN
              { 0x47, 0x00 }, // MIC1_LP
              { 0x48, 0x00 }, // MIC2_LP
              { 0x49, 0x00 }, // MIC3_LP
              { 0x4A, 0x00 }, // MIC4_LP
              { 0x4B, 0x00 }, // MIC12_PDN
              { 0x4C, 0xFF }, // MIC34_PDN
              { 0x01, 0x14 }, // CLK_ON_OFF
            };
            for (auto& d: data)
            {
              es7210_write_reg(d.reg, d.value);
            }
          }
/*
uint8_t buf[0x50];
for (int i = 0; i < 0x50; ++i)
{
  self->In_I2C.readRegister(es7210_i2c_addr, i, &buf[i], 1, 400000);
  if ((i & 15) == 15)
  {
    auto d = &buf[i-15];
    ESP_LOGE("DEBUG","%02x  :%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x"
    ,i>>4 , d[ 0], d[ 1], d[ 2], d[ 3], d[ 4], d[ 5], d[ 6], d[ 7], d[ 8], d[ 9], d[10], d[11], d[12], d[13], d[14], d[15]);
  }
}
//*/
        }
      }
      break;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      self->Power.Axp192.setLDO0(enabled ? 2800 : 0);
      break;

#endif
    default:
      break;
    }
    return true;
  }

#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
  static constexpr gpio_num_t TFCARD_CS_PIN          = GPIO_NUM_4;
  static constexpr gpio_num_t CoreInk_BUTTON_EXT_PIN = GPIO_NUM_5;
  static constexpr gpio_num_t CoreInk_BUTTON_PWR_PIN = GPIO_NUM_27;
#endif

  board_t M5Unified::_check_boardtype(board_t board)
  {
#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
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

#if defined ( ARDUINO_M5STACK_CORE_ESP32 ) || defined ( ARDUINO_M5STACK_FIRE ) || defined ( ARDUINO_M5Stack_Core_ESP32 )

        board = board_t::board_M5Stack;

#elif defined ( ARDUINO_M5STACK_CORE2 ) || defined ( ARDUINO_M5STACK_Core2 )

        board = board_t::board_M5StackCore2;

#elif defined ( ARDUINO_M5STICK_C ) || defined ( ARDUINO_M5Stick_C )

        board = board_t::board_M5StickC;

#elif defined ( ARDUINO_M5STICK_C_PLUS ) || defined ( ARDUINO_M5Stick_C_Plus )

        board = board_t::board_M5StickCPlus;

#elif defined ( ARDUINO_M5STACK_COREINK ) || defined ( ARDUINO_M5Stack_CoreInk )

        board = board_t::board_M5StackCoreInk;

#elif defined ( ARDUINO_M5STACK_PAPER ) || defined ( ARDUINO_M5STACK_Paper )

        board = board_t::board_M5Paper;

#elif defined ( ARDUINO_M5STACK_TOUGH )

        board = board_t::board_M5Tough;

#elif defined ( ARDUINO_M5STACK_ATOM ) || defined ( ARDUINO_M5Stack_ATOM )

        board = board_t::board_M5Atom;

#elif defined ( ARDUINO_M5STACK_TIMER_CAM ) || defined ( ARDUINO_M5Stack_Timer_CAM )

        board = board_t::board_M5TimerCam;

#endif
        break;
      }
    }

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    if (board == board_t::board_unknown)
    { /// StampS3 or AtomS3Lite,S3U ?
      ///   After setting GPIO38 to INPUT PULL-UP, change to INPUT and read it.
      ///   In the case of STAMPS3: Returns 0. Charge is sucked by SGM2578.
      ///   In the case of ATOMS3Lite/S3U : Returns 1. Charge remains. ( Since it is not connected to anywhere. )
      ///
      /// AtomS3Lite or AtomS3U ?
      ///   After setting GPIO4 to INPUT PULL-UP, read it.
      ///   In the case of ATOMS3Lite : Returns 0. Charge is sucked by InfraRed.
      ///   In the case of ATOMS3U    : Returns 1. Charge remains. ( Since it is not connected to anywhere. )

      m5gfx::gpio::pin_backup_t pin_backup[] = { GPIO_NUM_4, GPIO_NUM_8, GPIO_NUM_10, GPIO_NUM_12, GPIO_NUM_38 };
      auto result = m5gfx::gpio::command(
        (const uint8_t[]) {
        m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_4,
        m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_12,
        m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_38,
        m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_8,
        m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_10,
        m5gfx::gpio::command_mode_input_pullup  , GPIO_NUM_4,
        m5gfx::gpio::command_mode_input_pullup  , GPIO_NUM_12,
        m5gfx::gpio::command_mode_input_pullup  , GPIO_NUM_38,
        m5gfx::gpio::command_read               , GPIO_NUM_8,
        m5gfx::gpio::command_read               , GPIO_NUM_10,
        m5gfx::gpio::command_read               , GPIO_NUM_4,
        m5gfx::gpio::command_read               , GPIO_NUM_12,
        m5gfx::gpio::command_read               , GPIO_NUM_38,
        m5gfx::gpio::command_mode_input         , GPIO_NUM_38,
        m5gfx::gpio::command_delay              , 1,
        m5gfx::gpio::command_read               , GPIO_NUM_38,
        m5gfx::gpio::command_end
        }
      );
      /// result には、command_read で得たGPIOの状態が1bitずつ4回分入っている。
      board = ((const board_t[])
        { //                                                      ↓StampS3 pattern↓
          board_t::board_unknown,     board_t::board_unknown,     board_t::board_M5StampS3, board_t::board_unknown,      // ← unknown
          board_t::board_M5AtomS3Lite,board_t::board_M5AtomS3Lite,board_t::board_M5StampS3, board_t::board_M5AtomS3Lite, // ← AtomS3Lite pattern
          board_t::board_M5AtomS3U,   board_t::board_M5AtomS3U,   board_t::board_M5StampS3, board_t::board_M5AtomS3U,    // ← AtomS3U pattern
          board_t::board_unknown,     board_t::board_unknown,     board_t::board_M5StampS3, board_t::board_unknown,      // ← unknown
        })[result&15];
      if (board == board_t::board_M5StampS3) {
        if ((result >> 3) == 0b110) { board = board_t::board_M5Capsule; }
      }
      for (auto &backup : pin_backup) {
        backup.restore();
      }
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C3)
    if (board == board_t::board_unknown)
    { // StampC3 or StampC3U ?
      uint32_t tmp = *((volatile uint32_t *)(IO_MUX_GPIO20_REG));
      m5gfx::pinMode(GPIO_NUM_20, m5gfx::pin_mode_t::input_pulldown);
      // StampC3 has a strong external pull-up on GPIO20, which is HIGH even when input_pulldown is set.
      // Therefore, if it is LOW, it is not StampC3 and can be assumed to be StampC3U.
      // However, even if it goes HIGH, something may be connected to GPIO20 by StampC3U, so it is treated as unknown.
      // The StampC3U determination uses the fallback_board setting.
      if (m5gfx::gpio_in(GPIO_NUM_20) == false)
      {
        board = board_t::board_M5StampC3U;
      }
      *((volatile uint32_t *)(IO_MUX_GPIO20_REG)) = tmp;
    }

#endif

    return board;
  }

  void M5Unified::_setup_i2c(board_t board)
  {
#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
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

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    i2c_port_t in_port = I2C_NUM_1;
    gpio_num_t in_sda = GPIO_NUM_38;
    gpio_num_t in_scl = GPIO_NUM_39;

    i2c_port_t ex_port = I2C_NUM_0;
    gpio_num_t ex_sda = GPIO_NUM_2;
    gpio_num_t ex_scl = GPIO_NUM_1;
    switch (board)
    {

    case board_t::board_M5StackCoreS3:
      in_sda = GPIO_NUM_12;
      in_scl = GPIO_NUM_11;
      break;

    case board_t::board_M5AtomS3:
    case board_t::board_M5AtomS3Lite:
    case board_t::board_M5AtomS3U:
      break;

    case board_t::board_M5StampS3:
      in_sda = GPIO_NUM_NC;
      in_scl = GPIO_NUM_NC;
      ex_sda = GPIO_NUM_13;
      ex_scl = GPIO_NUM_15;
      break;

    case board_t::board_M5Capsule:
      in_sda = GPIO_NUM_8;
      in_scl = GPIO_NUM_10;
      ex_sda = GPIO_NUM_13;
      ex_scl = GPIO_NUM_15;
      break;

    case board_t::board_M5Dial:
    case board_t::board_M5DinMeter:
      in_sda = GPIO_NUM_11;
      in_scl = GPIO_NUM_12;
      ex_sda = GPIO_NUM_13;
      ex_scl = GPIO_NUM_15;
      break;

    default:
      break;
    }
    if ((int)in_sda >= 0)
    {
      In_I2C.begin(in_port, in_sda, in_scl);
    }
    if ((int)ex_sda >= 0)
    {
      Ex_I2C.setPort(ex_port, ex_sda, ex_scl);
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C3)
    /// StampC3 does not have internal i2c.
    In_I2C.setPort(-1, -1, -1);

    { /// setup External I2C
      i2c_port_t ex_port = I2C_NUM_0;
      gpio_num_t ex_sda = GPIO_NUM_1;
      gpio_num_t ex_scl = GPIO_NUM_0;
      Ex_I2C.setPort(ex_port, ex_sda, ex_scl);
    }
#endif
  }

  void M5Unified::_begin(const config_t& cfg)
  {
    /// setup power management ic
    Power.begin();
    Power.setExtOutput(cfg.output_power);
    if (cfg.led_brightness)
    {
      M5.Power.setLed(cfg.led_brightness);
    }
    if (Power.getType() == Power_Class::pmic_t::pmic_axp2101
     || Power.getType() == Power_Class::pmic_t::pmic_axp192)
    {
      use_pmic_button = cfg.pmic_button;
      /// Slightly lengthen the acceptance time of the AXP192 power button multiclick.
      BtnPWR.setHoldThresh(BtnPWR.getHoldThresh() * 1.2);
    }

    if (cfg.clear_display)
    {
      Display.clear();
    }

#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    switch (_board)
    {
    case board_t::board_M5Stack:
      // Countermeasure to the problem that GPIO15 affects WiFi sensitivity when M5GO bottom is connected.
      m5gfx::pinMode(GPIO_NUM_15, m5gfx::pin_mode_t::output);
      m5gfx::gpio_lo(GPIO_NUM_15);

      // M5Stack Core v2.6 has a problem that SPI communication speed cannot be increased.
      // This problem can be solved by increasing the GPIO drive current.
      // ※  This allows SunDisk SD cards to communicate at 20 MHz. (without M5GO bottom.)
      //     This allows communication with ModuleDisplay at 80 MHz.
      for (auto gpio: (const gpio_num_t[]){ GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_23 })
      {
        *(volatile uint32_t*)(GPIO_PIN_MUX_REG[gpio]) |= FUN_DRV_M; // gpio drive current set to 40mA.
        gpio_pulldown_dis(gpio); // disable pulldown.
        gpio_pullup_en(gpio);    // enable pullup.
      }
      break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
    case board_t::board_M5Atom:
    case board_t::board_M5AtomU:
      // Countermeasure to the problem that CH552 applies 4v to GPIO0, thus reducing WiFi sensitivity.
      // Setting output_high adds a bias of 3.3v and suppresses overvoltage.
      m5gfx::pinMode(GPIO_NUM_0, m5gfx::pin_mode_t::output);
      m5gfx::gpio_hi(GPIO_NUM_0);
      break;

    default:
      break;
    }
#endif

#if defined ( ARDUINO )

    if (cfg.serial_baudrate)
    { // Wait with delay to prevent startup log output from disappearing.
      delay(16);
      Serial.begin(cfg.serial_baudrate);
    }
    if (cfg.serial1_baudrate)
    {
      delay(16);
      Serial1.begin(cfg.serial1_baudrate);
    }
    if (cfg.serial2_baudrate)
    {
      delay(16);
      Serial2.begin(cfg.serial2_baudrate);
    }

#endif

    switch (_board) /// setup Hardware Buttons
    {
#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
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

    case board_t::board_M5StickCPlus2:
      m5gfx::pinMode(GPIO_NUM_35, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_37, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_39, m5gfx::pin_mode_t::input);
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32C3)

    case board_t::board_M5StampC3:
      m5gfx::pinMode(GPIO_NUM_3, m5gfx::pin_mode_t::input_pullup);
      break;

    case board_t::board_M5StampC3U:
      m5gfx::pinMode(GPIO_NUM_9, m5gfx::pin_mode_t::input_pullup);
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5AtomS3:
    case board_t::board_M5AtomS3Lite:
    case board_t::board_M5AtomS3U:
      m5gfx::pinMode(GPIO_NUM_41, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5StampS3:
      m5gfx::pinMode(GPIO_NUM_0, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5Capsule:
    case board_t::board_M5Dial:
    case board_t::board_M5DinMeter:
      m5gfx::pinMode(GPIO_NUM_42, m5gfx::pin_mode_t::input);
      break;

#endif

    default:
      break;
    }
  }

  void M5Unified::_begin_spk(config_t& cfg)
  {
    if (cfg.internal_mic)
    {
      auto mic_cfg = Mic.config();

      mic_cfg.over_sampling = 1;
      mic_cfg.i2s_port = I2S_NUM_0;
      switch (_board)
      {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5StackCoreS3:
        if (cfg.internal_mic)
        {
          mic_cfg.magnification = 2;
          mic_cfg.over_sampling = 1;
          mic_cfg.pin_mck = GPIO_NUM_0;
          mic_cfg.pin_bck = GPIO_NUM_34;
          mic_cfg.pin_ws = GPIO_NUM_33;
          mic_cfg.pin_data_in = GPIO_NUM_14;
          mic_cfg.i2s_port = I2S_NUM_1;
          mic_cfg.stereo = true;
        }
        break;

      case board_t::board_M5AtomS3U:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_data_in = GPIO_NUM_38;
          mic_cfg.pin_ws = GPIO_NUM_39;
        }
        break;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
      case board_t::board_M5Stack:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_data_in = GPIO_NUM_34;  // M5GO bottom MIC
          mic_cfg.i2s_port = I2S_NUM_0;
          mic_cfg.use_adc = true;    // use ADC analog input
          mic_cfg.over_sampling = 4;
        }
        break;

      case board_t::board_M5StickC:
      case board_t::board_M5StickCPlus:
      case board_t::board_M5StickCPlus2:
      case board_t::board_M5Tough:
      case board_t::board_M5StackCore2:
        if (cfg.internal_mic)
        { /// builtin PDM mic
          mic_cfg.pin_data_in = GPIO_NUM_34;
          mic_cfg.pin_ws = GPIO_NUM_0;
        }
        break;

      case board_t::board_M5AtomU:
        { /// ATOM U builtin PDM mic
          mic_cfg.pin_data_in = GPIO_NUM_19;
          mic_cfg.pin_ws = GPIO_NUM_5;
        }
        break;

      case board_t::board_M5Atom:
        { /// ATOM ECHO builtin PDM mic
          mic_cfg.pin_data_in = GPIO_NUM_23;
          mic_cfg.pin_ws = GPIO_NUM_33;
        }
        break;
#endif
      default:
        break;
      }
      if (mic_cfg.pin_data_in >= 0)
      {
        Mic.setCallback(this, _microphone_enabled_cb);
        Mic.config(mic_cfg);
      }
    }

    if (cfg.external_spk_detail.enabled && cfg.external_speaker_value == 0) {
      cfg.external_speaker.atomic_spk = false==cfg.external_spk_detail.omit_atomic_spk;
      cfg.external_speaker.hat_spk = false==cfg.external_spk_detail.omit_spk_hat;
    }

    if (cfg.internal_spk || cfg.external_speaker_value)
    {
      auto spk_cfg = Speaker.config();
      // set default speaker gain.
      spk_cfg.magnification = 16;
#if defined SOC_I2S_NUM
      spk_cfg.i2s_port = (i2s_port_t)(SOC_I2S_NUM - 1);
#else
      spk_cfg.i2s_port = (i2s_port_t)(I2S_NUM_MAX - 1);
#endif
      switch (_board)
      {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5StackCoreS3:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_bck = GPIO_NUM_34;
          spk_cfg.pin_ws = GPIO_NUM_33;
          spk_cfg.pin_data_out = GPIO_NUM_13;
          spk_cfg.magnification = 4;
          spk_cfg.i2s_port = I2S_NUM_1;
        }
        break;

      case board_t::board_M5AtomS3:
      case board_t::board_M5AtomS3Lite:
        if (cfg.external_speaker.atomic_spk && (Display.getBoard() != board_t::board_M5AtomDisplay))
        { // for ATOMIC SPK
          m5gfx::pinMode(GPIO_NUM_6, m5gfx::pin_mode_t::input_pulldown); // MOSI
          m5gfx::pinMode(GPIO_NUM_7, m5gfx::pin_mode_t::input_pulldown); // SCLK
          if (m5gfx::gpio_in(GPIO_NUM_6)
            && m5gfx::gpio_in(GPIO_NUM_7))
          {
            ESP_LOGD("M5Unified", "ATOMIC SPK");
            cfg.internal_imu = false; /// avoid conflict with i2c
            cfg.internal_rtc = false; /// avoid conflict with i2c
            spk_cfg.pin_bck = GPIO_NUM_5;
            spk_cfg.pin_ws = GPIO_NUM_39;
            spk_cfg.pin_data_out = GPIO_NUM_38;
            spk_cfg.magnification = 16;
          }
        }
        break;

      case board_t::board_M5Capsule:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_2;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

      case board_t::board_M5Dial:
      case board_t::board_M5DinMeter:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_3;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
      case board_t::board_M5Stack:
        if (cfg.internal_spk)
        {
          m5gfx::gpio_lo(GPIO_NUM_25);
          m5gfx::pinMode(GPIO_NUM_25, m5gfx::pin_mode_t::output);
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_cfg.use_dac = true;
          spk_cfg.pin_data_out = GPIO_NUM_25;
          spk_cfg.magnification = 8;
        }
        break;

      case board_t::board_M5StackCoreInk:
      case board_t::board_M5StickCPlus:
      case board_t::board_M5StickCPlus2:
        if (cfg.internal_spk)
        {
          spk_cfg.buzzer = true;
          spk_cfg.pin_data_out = GPIO_NUM_2;
          spk_cfg.magnification = 48;
        }
        NON_BREAK;

      case board_t::board_M5StickC:
        if (cfg.external_speaker.hat_spk2 && (_board != board_t::board_M5StackCoreInk))
        { /// for HAT SPK2 (for StickC/StickCPlus.  CoreInk does not support.)
          spk_cfg.pin_data_out = GPIO_NUM_25;
          spk_cfg.pin_bck = GPIO_NUM_26;
          spk_cfg.pin_ws = GPIO_NUM_0;
          spk_cfg.i2s_port = I2S_NUM_1;
          spk_cfg.use_dac = false;
          spk_cfg.buzzer = false;
          spk_cfg.magnification = 16;
        }
        else if (cfg.external_speaker.hat_spk)
        { /// for HAT SPK
          use_hat_spk = true;
          gpio_num_t pin_en = _board == board_t::board_M5StackCoreInk ? GPIO_NUM_25 : GPIO_NUM_0;
          m5gfx::gpio_lo(pin_en);
          m5gfx::pinMode(pin_en, m5gfx::pin_mode_t::output);
          m5gfx::gpio_lo(GPIO_NUM_26);
          m5gfx::pinMode(GPIO_NUM_26, m5gfx::pin_mode_t::output);
          spk_cfg.pin_data_out = GPIO_NUM_26;
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_cfg.use_dac = true;
          spk_cfg.buzzer = false;
          spk_cfg.magnification = 32;
        }
        break;

      case board_t::board_M5Tough:
        // The magnification is set higher than Core2 here because the waterproof case reduces the sound.;
        spk_cfg.magnification = 24;
        NON_BREAK;
      case board_t::board_M5StackCore2:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_bck = GPIO_NUM_12;
          spk_cfg.pin_ws = GPIO_NUM_0;
          spk_cfg.pin_data_out = GPIO_NUM_2;
        }
        break;

      case board_t::board_M5Atom:
        if (cfg.internal_spk && (Display.getBoard() != board_t::board_M5AtomDisplay))
        { // for ATOM ECHO
          spk_cfg.pin_bck = GPIO_NUM_19;
          spk_cfg.pin_ws = GPIO_NUM_33;
          spk_cfg.pin_data_out = GPIO_NUM_22;
          spk_cfg.magnification = 12;
        }
        NON_BREAK;
      case board_t::board_M5AtomPsram:
        if (cfg.external_speaker.atomic_spk && (Display.getBoard() != board_t::board_M5AtomDisplay))
        { // for ATOMIC SPK
          // 19,23 pulldown read check ( all high = ATOMIC_SPK ? ) // MISO is not used for judgment as it changes depending on the state of the SD card.
          gpio_num_t pin = (_board == board_t::board_M5AtomPsram) ? GPIO_NUM_5 : GPIO_NUM_23;
          m5gfx::pinMode(GPIO_NUM_19, m5gfx::pin_mode_t::input_pulldown); // MOSI
          m5gfx::pinMode(pin        , m5gfx::pin_mode_t::input_pulldown); // SCLK
          if (m5gfx::gpio_in(GPIO_NUM_19)
            && m5gfx::gpio_in(pin        ))
          {
            ESP_LOGD("M5Unified", "ATOMIC SPK");
            cfg.internal_imu = false; /// avoid conflict with i2c
            cfg.internal_rtc = false; /// avoid conflict with i2c
            spk_cfg.pin_bck = GPIO_NUM_22;
            spk_cfg.pin_ws = GPIO_NUM_21;
            spk_cfg.pin_data_out = GPIO_NUM_25;
            spk_cfg.magnification = 16;
            auto mic = Mic.config();
            mic.pin_data_in = -1;   // disable mic for ECHO
            Mic.config(mic);
          }
        }
        break;
#endif
      default:
        break;
      }

      if (cfg.external_speaker_value)
      {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined ( CONFIG_IDF_TARGET_ESP32S3 )
 #define ENABLE_M5MODULE
        if (_board == board_t::board_M5StackCoreS3)
#elif defined ( CONFIG_IDF_TARGET_ESP32 ) || !defined ( CONFIG_IDF_TARGET )
 #define ENABLE_M5MODULE
        if (  _board == board_t::board_M5Stack
          || _board == board_t::board_M5StackCore2
          || _board == board_t::board_M5Tough)
#endif
        {
#ifdef ENABLE_M5MODULE
          bool use_module_display = cfg.external_speaker.module_display
                                && (0 <= getDisplayIndex(m5gfx::board_M5ModuleDisplay));
          if (use_module_display || cfg.external_speaker.module_rca)
          {
            if (use_module_display) {
              spk_cfg.sample_rate = 48000; // Module Display audio output is fixed at 48 kHz
            }
            uint32_t pins_index = use_module_display;
  #if defined ( CONFIG_IDF_TARGET_ESP32S3 )
            static constexpr const uint8_t pins[][2] =
            {// DOUT       , BCK
              { GPIO_NUM_13, GPIO_NUM_7 }, // CoreS3 + ModuleRCA
              { GPIO_NUM_13, GPIO_NUM_6 }, // CoreS3 + ModuleDisplay
            };
  #else
            static constexpr const uint8_t pins[][2] =
            {// DOUT       , BCK
              { GPIO_NUM_2 , GPIO_NUM_19 }, // Core2 and Tough + ModuleRCA
              { GPIO_NUM_2 , GPIO_NUM_27 }, // Core2 and Tough + ModuleDisplay
              { GPIO_NUM_15, GPIO_NUM_13 }, // Core + ModuleRCA
              { GPIO_NUM_15, GPIO_NUM_12 }, // Core + ModuleDisplay
            };
            // !core is (Core2 + Tough)
            if (_board == m5::board_t::board_M5Stack) {
              pins_index += 2;            
            }
  #endif
            spk_cfg.pin_data_out = pins[pins_index][0];
            spk_cfg.pin_bck      = pins[pins_index][1];
            spk_cfg.i2s_port = I2S_NUM_1;
            spk_cfg.magnification = 16;
            spk_cfg.stereo = true;
            spk_cfg.buzzer = false;
            spk_cfg.use_dac = false;
            spk_cfg.pin_ws = GPIO_NUM_0;     // LRCK
          }
 #undef ENABLE_M5MODULE
#endif
        }
      }

      if (spk_cfg.pin_data_out >= 0)
      {
        Speaker.setCallback(this, _speaker_enabled_cb);
        Speaker.config(spk_cfg);
      }
    }
  }

  bool M5Unified::_begin_rtc_imu(const config_t& cfg)
  {
    bool port_a_used = false;
    if (cfg.external_rtc || cfg.external_imu)
    {
      M5.Ex_I2C.begin();
    }

    if (cfg.internal_rtc && In_I2C.isEnabled())
    {
      M5.Rtc.begin();
    }
    if (!M5.Rtc.isEnabled() && cfg.external_rtc)
    {
      port_a_used = M5.Rtc.begin(&M5.Ex_I2C);
    }
    if (M5.Rtc.isEnabled())
    {
      M5.Rtc.setSystemTimeFromRtc();
      if (cfg.disable_rtc_irq) {
        M5.Rtc.disableIRQ();
      }
    }

    if (cfg.internal_imu && In_I2C.isEnabled())
    {
      M5.Imu.begin(&M5.In_I2C, M5.getBoard());
    }
    if (!M5.Imu.isEnabled() && cfg.external_imu)
    {
      port_a_used = M5.Imu.begin(&M5.Ex_I2C) || port_a_used;
    }
    return port_a_used;
  }

  void M5Unified::update( void )
  {
    auto ms = m5gfx::millis();
    _updateMsec = ms;

    if (Touch.isEnabled())
    {
      Touch.update(ms);
    }

#if defined (M5UNIFIED_PC_BUILD)
    BtnA.setRawState(ms, !m5gfx::gpio_in(39));
    BtnB.setRawState(ms, !m5gfx::gpio_in(38));
    BtnC.setRawState(ms, !m5gfx::gpio_in(37));
    BtnPWR.setRawState(ms, !m5gfx::gpio_in(36));
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    uint_fast8_t raw_gpio32_39 = ~GPIO.in1.data;
 // uint_fast8_t raw_gpio37_40 = ~GPIO.in1.data >> 5;
    uint_fast8_t btn_bits = 0;
    switch (_board)
    {
    case board_t::board_M5StackCore2:
      {
        int i = Touch.getCount();
        while (--i >= 0)
        {
          auto raw = Touch.getTouchPointRaw(i);
          if (raw.y > 240)
          {
            auto det = Touch.getDetail(i);
            if (det.state & touch_state_t::touch)
            {
              if (BtnA.isPressed()) { btn_bits |= 1 << 0; }
              if (BtnB.isPressed()) { btn_bits |= 1 << 1; }
              if (BtnC.isPressed()) { btn_bits |= 1 << 2; }
              if (btn_bits || !(det.state & touch_state_t::mask_moving))
              {
                btn_bits |= 1 << ((raw.x - 2) / 107);
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
      btn_bits = (raw_gpio32_39 >> 5) & 0x07; // gpio37 A / gpio38 B / gpio39 C
      break;

    case board_t::board_M5Stack:
      btn_bits = ((raw_gpio32_39 >> 5) & 0x02)  // gpio38 B
               + ((raw_gpio32_39 >> 3) & 0x04); // gpio37 C
      NON_BREAK; /// don't break;

    case board_t::board_M5Atom:
    case board_t::board_M5AtomPsram:
    case board_t::board_M5AtomU:
    case board_t::board_M5StampPico:
      btn_bits += (raw_gpio32_39 >> 7) & 0x01; // gpio39 A
      break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      btn_bits = ((raw_gpio32_39 >> 5) & 0x01)  // gpio37 A
               + ((raw_gpio32_39 >> 6) & 0x02); // gpio39 B
      break;

    case board_t::board_M5StickCPlus2:
      btn_bits = ((raw_gpio32_39 >> 5) & 0x01)  // gpio37 A
               + ((raw_gpio32_39 >> 6) & 0x02); // gpio39 B
      BtnPWR.setRawState(ms, raw_gpio32_39 & 0x08); // gpio35 PWR
      break;

    default:
      break;
    }

    BtnA.setRawState(ms, btn_bits & 1);
    BtnB.setRawState(ms, btn_bits & 2);
    BtnC.setRawState(ms, btn_bits & 4);
    if (use_pmic_button)
    {
      Button_Class::button_state_t state = Button_Class::button_state_t::state_nochange;
      bool read_axp = (ms - BtnPWR.getUpdateMsec()) >= BTNPWR_MIN_UPDATE_MSEC;
      if (read_axp || BtnPWR.getState())
      {
        switch (Power.getKeyState())
        {
        case 0: break;
        case 2:   state = Button_Class::button_state_t::state_clicked; break;
        default:  state = Button_Class::button_state_t::state_hold;    break;
        }
        BtnPWR.setState(ms, state);
      }
    }

#elif defined (CONFIG_IDF_TARGET_ESP32S3)

    switch (_board)
    {
    case board_t::board_M5AtomS3:
    case board_t::board_M5AtomS3Lite:
    case board_t::board_M5AtomS3U:
      BtnA.setRawState(ms, !m5gfx::gpio_in(GPIO_NUM_41));
      break;

    case board_t::board_M5StampS3:
      BtnA.setRawState(ms, !m5gfx::gpio_in(GPIO_NUM_0));
      break;

    case board_t::board_M5Capsule:
    case board_t::board_M5Dial:
    case board_t::board_M5DinMeter:
      BtnA.setRawState(ms, !m5gfx::gpio_in(GPIO_NUM_42));
      break;

    default:

      if (use_pmic_button)
      {
        Button_Class::button_state_t state = Button_Class::button_state_t::state_nochange;
        bool read_axp = (ms - BtnPWR.getUpdateMsec()) >= BTNPWR_MIN_UPDATE_MSEC;
        if (read_axp || BtnPWR.getState())
        {
          switch (Power.getKeyState())
          {
          case 0: break;
          case 2:   state = Button_Class::button_state_t::state_clicked; break;
          default:  state = Button_Class::button_state_t::state_hold;    break;
          }
          BtnPWR.setState(ms, state);
        }
      }
      break;
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

  M5GFX& M5Unified::getDisplay(size_t index)
  {
    return index != _primary_display_index && index < this->_displays.size() ? this->_displays[index] : Display;
  }

  std::size_t M5Unified::addDisplay(M5GFX& dsp)
  {
    this->_displays.push_back(dsp);
    auto res = this->_displays.size() - 1;
    setPrimaryDisplay(res == 0 ? 0 : _primary_display_index);

    // Touch screen operation is always limited to the first display.
    Touch.begin(_displays.front().touch() ? &_displays.front() : nullptr);

    return res;
  }

  int32_t M5Unified::getDisplayIndex(m5gfx::board_t board) {
    int i = 0;
    for (auto &d : _displays)
    {
      if (board == d.getBoard()) { return i; }
      ++i;
    }
    return -1;
  }

  int32_t M5Unified::getDisplayIndex(std::initializer_list<m5gfx::board_t> board_list)
  {
    for (auto b : board_list)
    {
      int32_t i = getDisplayIndex(b);
      if (i >= 0) { return i; }
    }
    return -1;
  }

  bool M5Unified::setPrimaryDisplay(std::size_t index)
  {
    if (index >= _displays.size()) { return false; }
    std::size_t pdi = _primary_display_index;

    if (pdi < _displays.size())
    {
      _displays[pdi] = Display;
    }
    _primary_display_index = index;
    Display = _displays[index];
    return true;
  }

  bool M5Unified::setPrimaryDisplayType(std::initializer_list<m5gfx::board_t> board_list)
  {
    auto i = getDisplayIndex(board_list);
    bool res = (i >= 0);
    if (res) { setPrimaryDisplay(i); }
    return res;
  }

  void M5Unified::setLogDisplayIndex(size_t index)
  {
    Log.setDisplay(getDisplay(index));
  }

  void M5Unified::setLogDisplayType(std::initializer_list<m5gfx::board_t> board_list)
  {
    auto i = getDisplayIndex(board_list);
    if (i >= 0) { Log.setDisplay(getDisplay(i)); }
  }
}
