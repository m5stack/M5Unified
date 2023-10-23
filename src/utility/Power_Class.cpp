// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Power_Class.hpp"

#include "../M5Unified.hpp"

#if !defined (M5UNIFIED_PC_BUILD)

#include <esp_log.h>
#include <esp_sleep.h>
#include <sdkconfig.h>

#include <esp_adc_cal.h>
#include <soc/adc_channel.h>

#if __has_include (<esp_idf_version.h>)
 #include <esp_idf_version.h>
 #if ESP_IDF_VERSION_MAJOR >= 4
  #define NON_BREAK ;[[fallthrough]];
 #endif
#endif

#endif

#ifndef NON_BREAK
#define NON_BREAK ;
#endif

namespace m5
{
  static constexpr const uint32_t i2c_freq = 100000;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
  static constexpr int aw9523_i2c_addr = 0x58;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
  static constexpr int CoreInk_POWER_HOLD_PIN = 12;
  static constexpr int M5Paper_POWER_HOLD_PIN =  2;
  static constexpr int TimerCam_POWER_HOLD_PIN = 33;
  static constexpr int TimerCam_LED_PIN = 2;
  static constexpr int M5Paper_EXT5V_ENABLE_PIN = 5;
  static constexpr int StickCPlus2_POWER_HOLD_PIN = 4;
  static constexpr int StickCPlus2_LED_PIN = 19;
#endif

  bool Power_Class::begin(void)
  {
    _pmic = pmic_t::pmic_unknown;

#if !defined (M5UNIFIED_PC_BUILD)
#if defined (CONFIG_IDF_TARGET_ESP32S3)

    /// setup power management ic
    switch (M5.getBoard())
    {
    default:
      break;

    case board_t::board_M5StackCoreS3:
      M5.In_I2C.bitOn(aw9523_i2c_addr, 0x03, 0b10000000, i2c_freq);  // SY7088 BOOST_EN
      _pmic = Power_Class::pmic_t::pmic_axp2101;
      Axp2101.begin();
      static constexpr std::uint8_t reg_data_array[] =
      { 0x90, 0xBF  // LDOS ON/OFF control 0
      , 0x92, 18 -5 // ALDO1 set to 1.8v // for AW88298
      , 0x93, 33 -5 // ALDO2 set to 3.3v // for ES7210
      , 0x94, 33 -5 // ALDO3 set to 3.3v // for camera
      , 0x95, 33 -5 // ALDO3 set to 3.3v // for TF card slot
      , 0x27, 0x00 // PowerKey Hold=1sec / PowerOff=4sec
      , 0x69, 0x11 // CHGLED setting
      , 0x10, 0x30 // PMU common config
      };
      Axp2101.writeRegister8Array(reg_data_array, sizeof(reg_data_array));
      break;

    case board_t::board_M5Dial:
      _pwrHoldPin = GPIO_NUM_46;
      break;

    case board_t::board_M5Capsule:
      _pwrHoldPin = GPIO_NUM_46;
      _batAdc = (adc1_channel_t) ADC1_GPIO6_CHANNEL;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 2.0f;
      break;

    case board_t::board_M5DinMeter:
      _pwrHoldPin = GPIO_NUM_46;
      NON_BREAK;
    case board_t::board_M5Cardputer:
      _batAdc = (adc1_channel_t) ADC1_GPIO10_CHANNEL;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 2.0f;
      break;
    }

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    /// setup power management ic
    switch (M5.getBoard())
    {
    default:
      break;

    case board_t::board_M5TimerCam:
      _pwrHoldPin = TimerCam_POWER_HOLD_PIN;
      m5gfx::pinMode(TimerCam_POWER_HOLD_PIN, m5gfx::pin_mode_t::output);
      m5gfx::gpio_hi(TimerCam_POWER_HOLD_PIN);
      m5gfx::pinMode(TimerCam_LED_PIN, m5gfx::pin_mode_t::output);
      m5gfx::gpio_lo(TimerCam_LED_PIN);  // system LED off
      _batAdc = (adc1_channel_t) ADC1_GPIO38_CHANNEL;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 1.513f;
      break;

    case board_t::board_M5StackCoreInk:
      _pwrHoldPin = CoreInk_POWER_HOLD_PIN;
      _wakeupPin = GPIO_NUM_27; // power button;
      _rtcIntPin = GPIO_NUM_19;
      _batAdc = (adc1_channel_t) ADC1_GPIO35_CHANNEL;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 25.1f / 5.1f;
      break;

    case board_t::board_M5Paper:
      _pwrHoldPin = M5Paper_POWER_HOLD_PIN;
      m5gfx::pinMode(M5Paper_EXT5V_ENABLE_PIN, m5gfx::pin_mode_t::output);
      _wakeupPin = GPIO_NUM_36; // touch panel INT;
      _batAdc = (adc1_channel_t) ADC1_GPIO35_CHANNEL;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 2.0f;
      break;

    case board_t::board_M5Tough:
    case board_t::board_M5StackCore2:
      _wakeupPin = GPIO_NUM_39; // touch panel INT;
      _pmic = Power_Class::pmic_t::pmic_axp192;
      break;

    case board_t::board_M5Station:
      m5gfx::pinMode(GPIO_NUM_12, m5gfx::pin_mode_t::output);
      _pmic = Power_Class::pmic_t::pmic_axp192;
      break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      _rtcIntPin = GPIO_NUM_35;
      _pmic = Power_Class::pmic_t::pmic_axp192;
      break;

    case board_t::board_M5StickCPlus2:
      _pwrHoldPin = StickCPlus2_POWER_HOLD_PIN;
      m5gfx::pinMode(StickCPlus2_LED_PIN, m5gfx::pin_mode_t::output);
      _batAdc = (adc1_channel_t) ADC1_GPIO38_CHANNEL;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 2.0f;
      break;

    case board_t::board_M5Stack:
      _pmic = pmic_t::pmic_ip5306;
      Ip5306.begin();
      {
        static constexpr std::uint8_t reg_data_array[] =
          ///       ++-------- reserved (00)
          ///       ||+------- boost enable
          ///       |||+------ charge enable
          ///       ||||+----- reserved (0)
          ///       |||||+---- Insert load auto power on function enable
          ///       ||||||+--- BOOST output normally open function  (※ DeepSleepやRTC Timer使用時は1にすること (負荷が軽いと電力供給を停止されてしまうため))
          ///       |||||||+-- Push button shutdown enable
          ///       ||||||||
          { 0x00, 0b00110001 // reg00h SYS_CTL0

          ///       +--------- Off boost control signal selection (1:Long press ; 0:Short press twice)
          ///       |+-------- Switch WLED flashlight control signal selection (1:Short press twice ; 0:Long press)
          ///       ||+------- Short press switch boost
          ///       |||++----- reserved(11)
          ///       |||||+---- Whether to turn on Boost after VIN is pulled out
          ///       ||||||+--- reserved(0)
          ///       |||||||+-- Batlow 3.0V Low Power Shutdown Enable
          ///       ||||||||
          , 0x01, 0b00011101 // reg01h SYS_CTL1

          ///       +++------- reserved(011)
          ///       |||+------ KEY Long press time setting (0:2s ; 1:3s)
          ///       ||||++---- Light load shutdown time setting (00:8s ; 01:32s ; 10:16s ; 11:64s)
          ///       ||||||++-- reserved(00)
          ///       ||||||||
          , 0x02, 0b01101100 // reg02h SYS_CTL2

          ///       ++++++---- reserved(0)
          ///       ||||||++-- Charge full stop setting  4.14/4.26/4.305/4.35
          ///       ||||||||
          , 0x20, 0b00000000 // reg20h

          ///       ++-------- Battery side stop charging current detection
          ///       ||+------- reserved(0)
          ///       |||+++---- Charging undervoltage loop setting (voltage at output VOUT during charging)
          ///       ||||||++-- reserved(01)
          ///       ||||||||
          , 0x21, 0b00001001 // reg21h Charger_CTL1 : 200mA ; 4.55V

          ///       ++++------ reserved(0000)
          ///       ||||++---- Battery voltage setting (00:4.2V ; 01:4.3V ; 10:4.35V ; 11:4.4V)
          ///       ||||||++-- Constant voltage charging voltage boost setting (00:nothing ; 01:14mV ; 10:28mV ; 11:42mV)
          ///       ||||||||
          , 0x22, 0b00000010 // reg22h Charger_CTL2 : setChargeVoltage 4.2V + boost 28mV

          ///       ++-------- reserved(10)
          ///       ||+------- Charging constant current loop selection. (1:CC at VIN side ; 0:CC at BAT side)
          ///       |||+++++-- reserved(01110)
          ///       ||||||||
          , 0x23, 0b10101110 // reg23h Charger_CTL3 : VIN CC

          ///       +++------- reserved(110)
          ///       |||+++++-- Charger (VIN side) current setting. (I:0.05+b0*0.1+b1*0.2+b2*0.4+b3*0.8+b4*1.6A)
          ///       ||||||||
          , 0x24, 0b11000001 // reg24h CHG_DIG_CTL0 : setChargeCurrent 150mA
          };
        Ip5306.writeRegister8Array(reg_data_array, sizeof(reg_data_array));
      }
      break;
    }

    if (_pmic == Power_Class::pmic_t::pmic_axp192) {
      if (!Axp192.begin()) {
        if (Axp2101.begin()) {
          _pmic = Power_Class::pmic_t::pmic_axp2101;
        }
      }
    }

    if (_pmic == Power_Class::pmic_t::pmic_axp192)
    {
      static constexpr std::uint8_t reg_data_array[] =
        { 0x26, 0x6A    // reg26h DCDC1 3350mV (ESP32 VDD)

        ///       +--------- VBUS-IPSOUT channel selection control signal when VBUS is available (0:The N_VBUSEN pin determines whether to open this channel. / 1: The VBUS-IPSOUT path can be selected to be opened regardless of the status of N_VBUSEN)
        ///       |+-------- VBUS VHOLD pressure limit control (0:disable ; 1:enable)
        ///       ||+++----- VHOLD setting (x 100mV + 4.0V ;  000:4.0V ～ 111:4.7V)
        ///       |||||+---- reserved(0)
        ///       ||||||+--- VBUS current limit control enable signal
        ///       |||||||+-- VBUS current limit control opens time limit flow selection (0:500mA ; 1:100mA)
        ///       ||||||||
        , 0x30, 0b00000010 // reg30h VBUS-IPSOUT Pass-Through Management

        ///       ++++------ reserved
        ///       ||||+----- PWRON short press wake-up function enable setting in Sleep mode.
        ///       |||||+++-- VOFF Settings (x 100mV + 2.6V   000:2.6V ～ 111:3.3V)
        ///       ||||||||
        , 0x31, 0b00000100 // reg31h VOFF Shutdown voltage setting ( 3.0V )

        ///       +--------- Shutdown control under mode A Writing 1 to this bit will turn off the output of the AXP192
        ///       |+-------- Battery monitoring function setting (0:off ; 1:on)
        ///       ||++------ CHGLED pin function setting (00:High resistance ; 01:25% 1Hz blinking ; 10:25% 4Hz blinking ; 11:Output low level)
        ///       ||||+----- CHGLED pin control settings (0: Controlled by charging function ; 1: Controlled by register REG 32HBit[5:4])
        ///       |||||+---- reserved(1)
        ///       ||||||++-- AXP192 Shutdown delay time after N_OE changes from low to high Delay time (00: 0.5S ; 01: 1S ; 10: 2S ; 11: 3S)
        ///       ||||||||
        , 0x32, 0b01000010 // reg32h Enable bat detection

        ///       +--------- Charge function enable control bit, including internal and external channels
        ///       |++------- Charging target voltage setting ( 00:4.1V ; 01:4.15V ; 10:4.2V ; 11:4.36V)
        ///       |||+------ Charging end current setting (0:End charging when charging current is less than 10% setting ; 1: End charging when charging current is less than 15% setting)
        ///       ||||++++-- Internal path charging current setting
        ///       ||||||||
        , 0x33, 0b11000000 // reg33h Charge control 1 (Charge 4.2V, 100mA)

        , 0x35, 0xA2    // reg35h Enable RTC BAT charge 
        , 0x36, 0x0C    // reg36h 128ms power on, 4s power off
        , 0x82, 0xFF    // reg82h ADC all on
        , 0x83, 0x80    // reg83h ADC temp on
        , 0x84, 0x32    // reg84h ADC 25Hz
        , 0x90, 0x07    // reg90h GPIO0(LDOio0) floating
        , 0x91, 0xA0    // reg91h GPIO0(LDOio0) 2.8V
        , 0x98, 0x00    // PWM1 X
        , 0x99, 0xFF    // PWM1 Y1
        , 0x9A, 0xFF    // PWM1 Y1

// 2023/06/12 以下の指定は削除。
//      , 0x12, 0x07    // reg12h enable DCDC1,DCDC3,LDO2  /  disable LDO3,DCDC2,EXTEN
// 理由:Core2本体リセット操作後、起動時にEXTEN disableとなって外部機器への電源供給が一瞬途切れるため。
        };
      Axp192.writeRegister8Array(reg_data_array, sizeof(reg_data_array));


      switch (M5.getBoard())
      {
      case board_t::board_M5StickC:
      case board_t::board_M5StickCPlus:
        Axp192.writeRegister8(0x30, 0b10000000); // reg30h VBUS-IPSOUT Pass-Through Management
        Axp192.setDCDC3(0);
        Axp192.setLDO3(3000); // LCD power
        break;

      case board_t::board_M5StackCore2:
        Axp192.setLDO2(3300); // LCD + SD peripheral power supply
        Axp192.setLDO3(0);    // VIB_MOTOR STOP
        Axp192.setGPIO2(false);   // SPEAKER STOP
        Axp192.writeRegister8(0x9A, 255);  // PWM 255 (LED OFF)
        Axp192.writeRegister8(0x92, 0x02); // GPIO1 PWM
        Axp192.setChargeCurrent(390); // Core2 battery = 390mAh
        break;

      case board_t::board_M5Tough:
        Axp192.setLDO2(3300); // LCD + SD peripheral
        Axp192.setGPIO2(false);   // SPEAKER STOP
        Axp192.setDCDC3(0);
        break;

      case board_t::board_M5Station:
        {
          static constexpr std::uint8_t reg92h_96h[] = 
          { 0x00 // GPIO1 NMOS OpenDrain
          , 0x00 // GPIO2 NMOS OpenDrain
          , 0x00 // GPIO0~2 low
          , 0x05 // GPIO3 NMOS OpenDrain, GPIO4 NMOS OpenDrain
          , 0x00 // GPIO3 low, GPIO4 low
          };
          Axp192.writeRegister(0x92, reg92h_96h, sizeof(reg92h_96h));
        }
        break;

      default:
        break;
      }
    }
    else if (_pmic == Power_Class::pmic_t::pmic_axp2101)
    {
      // for Core2 v1.1
      static constexpr std::uint8_t reg_data_array[] =
      { 0x27, 0x00 // PowerKey Hold=1sec / PowerOff=4sec
      , 0x10, 0x30 // PMU common config (internal off-discharge enable)
      , 0x12, 0x00 // BATFET disable
      , 0x68, 0x01 // Battery detection enabled.
      , 0x69, 0x13 // CHGLED setting
      , 0x99, 0x00 // DLDO1 set 0.5v (vibration motor)
      // , 0x18, 0x0E
      };
      Axp2101.writeRegister8Array(reg_data_array, sizeof(reg_data_array));

      // for Core2 v1.1 (AXP2101+INA3221)
      if (Ina3221.begin())
      {}
    }


#endif

    if (_pwrHoldPin < GPIO_NUM_MAX)
    {
      gpio_hold_en( (gpio_num_t)_pwrHoldPin );
    }
#endif
    return (_pmic != pmic_t::pmic_unknown);
  }

#if defined (CONFIG_IDF_TARGET_ESP32S3)

  static constexpr const uint32_t _core_s3_bus_en = 0b00000010; // BUS EN
  static constexpr const uint32_t _core_s3_usb_en = 0b00100000; // USB OTG EN
  static void _core_s3_output(uint8_t mask, bool enable)
  {
    static constexpr const uint8_t port0_reg = 0x02;
    //static constexpr const uint8_t port1_reg = 0x03;
    static constexpr const uint32_t port1_bitmask_boost = 0b10000000; // BOOST_EN

    uint8_t buf[2];

    if (M5.In_I2C.readRegister(aw9523_i2c_addr, port0_reg, buf, sizeof(buf), i2c_freq))
    {
      uint8_t p0 = buf[0] | mask;
      uint8_t p1 = buf[1] | port1_bitmask_boost;

      if (!enable) {
        p0 = buf[0] & ~mask;
        // if (0 == (p0 & (_core_s3_bus_en | _core_s3_usb_en))) // 両方が無効の場合のみ BOOST_EN を無効化する
        if (0 == (p0 & _core_s3_bus_en))
        {
          p1 &= ~port1_bitmask_boost;
        }
      }
// M5.Display.printf("%02x %02x\n", p0, p1);
      buf[0] = p0;
      buf[1] = p1;
      M5.In_I2C.writeRegister(aw9523_i2c_addr, port0_reg, buf, sizeof(buf), i2c_freq);
      // M5.In_I2C.writeRegister8(aw9523_i2c_addr, port0_reg, p0, i2c_freq);
      // M5.In_I2C.writeRegister8(aw9523_i2c_addr, port1_reg, p1, i2c_freq);
    }
//      Axp2101.setReg0x20Bit0(enable);
  }

#endif

  void Power_Class::setExtOutput(bool enable, ext_port_mask_t port_mask)
  {
    switch (M5.getBoard())
    {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StackCoreS3:
      {
        _core_s3_output(_core_s3_bus_en, enable);
      }
      break;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case board_t::board_M5Paper:
      if (enable) { m5gfx::gpio_hi(M5Paper_EXT5V_ENABLE_PIN); }
      else        { m5gfx::gpio_lo(M5Paper_EXT5V_ENABLE_PIN); }
      break;

    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
      {
        bool cancel = false;
        if (_pmic == pmic_axp2101) {
          cancel = (enable && (Ina3221.getShuntVoltage(0) < 0.0f || Ina3221.getShuntVoltage(1) < 0.0f) && (8 >= Axp2101.getBatteryLevel()));
          if (!cancel) {
            Axp2101.setBLDO2(enable * 3300);
            break;
          }
        } else {
          // If ACIN is false and VBUS current is detected and the battery is low, power supply from Core to the outside is inhibited.
          // This is because supplying power externally consumes battery power when there is no power supply from ACIN and power is supplied from VBUS.
          // ※ If receiving power from M-Bus, PortA, etc., there is no need to setExtPower to true.
          cancel = (enable && !Axp192.isACIN() && (0.0f < Axp192.getVBUSCurrent()) && (8 >= Axp192.getBatteryLevel()));
          if (!cancel) {
            Axp192.writeRegister8(0x90, enable ? 0x02 : 0x07); // GPIO0 : enable=LDO / disable=float
          }
        }
        if (cancel)
        {
          ESP_LOGW("Power","setExtPower(true) is canceled.");
          break;
        }
      }
      NON_BREAK;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      Axp192.setEXTEN(enable);
      break;

    case board_t::board_M5Station:
      for (int i = 0; i < 5; ++i)
      {
        if (port_mask & (1 << i)) { Axp192.setGPIO(i, enable); }
      }
      if (port_mask & ext_port_mask_t::ext_USB)
      {
        if (enable) { m5gfx::gpio_hi(GPIO_NUM_12); } // GPIO12 = M5Station USB power control
        else        { m5gfx::gpio_lo(GPIO_NUM_12); }
      }
      if (port_mask & ext_port_mask_t::ext_MAIN)
      {
        Axp192.setEXTEN(enable);
      }
#endif

    default:
      break;
    }
  }

  bool Power_Class::getExtOutput(void)
  {
    switch (M5.getBoard())
    {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StackCoreS3:
      {
        static constexpr const uint32_t port0_bitmask = 0b00000010; // BUS EN
        static constexpr const uint8_t port0_reg = 0x02;
        return M5.In_I2C.readRegister8(aw9523_i2c_addr, port0_reg, i2c_freq) & port0_bitmask;
      }
      break;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case board_t::board_M5Paper:
      return m5gfx::gpio_in(M5Paper_EXT5V_ENABLE_PIN);
      break;

    case board_t::board_M5StackCore2:
      if (_pmic == pmic_axp2101) {
        return Axp2101.getBLDO2Enabled();
      }
      NON_BREAK;

    case board_t::board_M5Tough:
    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
    case board_t::board_M5Station:
      return Axp192.getEXTEN();
      break;
#endif

    default:
      break;
    }
    return false;
  }

  void Power_Class::setUsbOutput(bool enable)
  {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    switch (M5.getBoard())
    {
    case board_t::board_M5StackCoreS3:
      _core_s3_output(_core_s3_usb_en, enable);
      break;

    default:
      break;
    }
#endif
  }

  bool Power_Class::getUsbOutput(void)
  {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    switch (M5.getBoard())
    {
    case board_t::board_M5StackCoreS3:
      {
        static constexpr const uint8_t reg = 0x02;
        return M5.In_I2C.readRegister8(aw9523_i2c_addr, reg, i2c_freq) & _core_s3_usb_en;
      }
      break;

    default:
      break;
    }
#endif
    return false;
  }

  void Power_Class::setLed(uint8_t brightness)
  {
#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    static std::unique_ptr<m5gfx::Light_PWM> led;

    switch (M5.getBoard())
    {
    case board_t::board_M5StackCore2:
      Axp192.writeRegister8(0x9A, 255-brightness);
      break;

    case board_t::board_M5StackCoreInk:
    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
    case board_t::board_M5StickCPlus2:
    case board_t::board_M5TimerCam:
      {
        if (led.get() == nullptr)
        {
          led.reset(new m5gfx::Light_PWM());
          auto cfg = led->config();
          cfg.invert = false;
          cfg.pwm_channel = 7;

          /// M5StickC,CPlus /CoreInk : LED = GPIO10 / TimerCam:LED = GPIO2
          switch (M5.getBoard()) {
          case board_t::board_M5StickCPlus2:
            cfg.pin_bl = StickCPlus2_LED_PIN;
            cfg.pwm_channel = 6; // ch6を選択 (ch7はLCDのバックライトに使用しているため)
            cfg.freq = 256;      // ※バックライトと同じ周波数を指定する(ch6とch7はタイマ周期が共通のため)
            break;

          case board_t::board_M5TimerCam:
            cfg.pin_bl = TimerCam_LED_PIN;
            break;

          default:
            cfg.invert = true;
            cfg.pin_bl = GPIO_NUM_10;
            break;
          }
          led->config(cfg);
          led->init(brightness);
        }
        led->setBrightness(brightness);
      }
      break;

    default:
      break;
    }
#endif
  }

  void Power_Class::_powerOff(bool withTimer)
  {
#if !defined (M5UNIFIED_PC_BUILD)
    bool use_deepsleep = true;
    if (withTimer && _rtcIntPin < GPIO_NUM_MAX)
    {
      gpio_num_t pin = (gpio_num_t)_rtcIntPin;
#if SOC_PM_SUPPORT_EXT_WAKEUP
      if (ESP_OK != esp_sleep_enable_ext0_wakeup( pin, false))
#endif
      {
        gpio_wakeup_enable( pin, gpio_int_type_t::GPIO_INTR_LOW_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        use_deepsleep = false;
      }
    }
    else
    {
      switch (_pmic)
      {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

      case pmic_t::pmic_axp192:
        Axp192.powerOff();
        break;

      case pmic_t::pmic_ip5306:
        Ip5306.setPowerBoostKeepOn(withTimer);
        break;

#endif

      case pmic_t::pmic_axp2101:
        Axp2101.powerOff();
        break;

#endif

      case pmic_t::pmic_unknown:
      default:
        break;
      }
    }

    if (_pwrHoldPin < GPIO_NUM_MAX)
    {
      m5gfx::gpio_lo( _pwrHoldPin );
    }

    if (use_deepsleep) { esp_deep_sleep_start(); }
    esp_light_sleep_start();
    esp_restart();
#endif
  }

  void Power_Class::_timerSleep(void)
  {
#if !defined (M5UNIFIED_PC_BUILD)

    if (_pwrHoldPin < GPIO_NUM_MAX)
    {
      gpio_hold_dis( (gpio_num_t)_pwrHoldPin );
    }
    M5.Display.sleep();
    M5.Display.waitDisplay();

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    switch (M5.getBoard())
    {
    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      /// RTCタイマーは指定時間になるとGPIO35をLOWにすることで通知を行うが、;
      /// 回路設計の問題でINTピン(GPIO35)がプルアップされておらず、そのままでは利用できない。;
      /// そのため、同じくGPIO35に接続されているMPU6886のINTピンを利用してプルアップを実施する。;
      /// IMUの種類がSH200Qの個体では対応できない (MPU6886が必要);
      m5gfx::pinMode(GPIO_NUM_35, m5gfx::pin_mode_t::input);
      for (int i = 1; i >= 0; --i) {
        M5.Imu.setINTPinActiveLogic(i);
        if (m5gfx::gpio_in(GPIO_NUM_35)) { break; }
      }
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
      esp_deep_sleep_start();
      return;
      break;

    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
      // esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0); // タッチパネルINT;
      // Axp192.powerOff();
      break;

    default:
      break;
    }
#endif
#endif
    _powerOff(true);
  }

  void Power_Class::deepSleep(std::uint64_t micro_seconds, bool touch_wakeup)
  {
    M5.Display.sleep();
#if !defined (M5UNIFIED_PC_BUILD)
    ESP_LOGD("Power","deepSleep");
#if defined (CONFIG_IDF_TARGET_ESP32C3)

#else

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (_pmic == pmic_t::pmic_ip5306)
    {
      Ip5306.setPowerBoostKeepOn(true);
    }
#endif

    uint_fast8_t wpin = _wakeupPin;
    if (touch_wakeup && wpin < GPIO_NUM_MAX)
    {
      esp_sleep_enable_ext0_wakeup((gpio_num_t)wpin, false);
      while (m5gfx::gpio_in(wpin) == false)
      {
        m5gfx::delay(10);
      }
    }
    if (micro_seconds > 0)
    {
      esp_sleep_enable_timer_wakeup(micro_seconds);
    }
    else
    {
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }
#endif
    esp_deep_sleep_start();
#endif
  }

  void Power_Class::lightSleep(std::uint64_t micro_seconds, bool touch_wakeup)
  {
#if !defined (M5UNIFIED_PC_BUILD)
    ESP_LOGD("Power","lightSleep");
#if defined (CONFIG_IDF_TARGET_ESP32C3)

#else

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (_pmic == pmic_t::pmic_ip5306)
    {
      Ip5306.setPowerBoostKeepOn(true);
    }
#endif

    uint_fast8_t wpin = _wakeupPin;
    if (touch_wakeup && wpin < GPIO_NUM_MAX)
    {
      esp_sleep_enable_ext0_wakeup((gpio_num_t)wpin, false);
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);
      while (m5gfx::gpio_in(wpin) == false)
      {
        m5gfx::delay(10);
      }
    }
    if (micro_seconds > 0){
      esp_sleep_enable_timer_wakeup(micro_seconds);
    }else{
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }
#endif
    esp_light_sleep_start();
#endif
  }

  void Power_Class::powerOff(void)
  {
    M5.Display.sleep();
    M5.Display.waitDisplay();
    _powerOff(false);
  }

  void Power_Class::timerSleep( int seconds )
  {
    M5.Rtc.disableIRQ();
    M5.Rtc.setAlarmIRQ(seconds);
#if !defined (M5UNIFIED_PC_BUILD)
    esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
#endif
    _timerSleep();
  }

  void Power_Class::timerSleep( const rtc_time_t& time)
  {
    M5.Rtc.disableIRQ();
    M5.Rtc.setAlarmIRQ(time);
    _timerSleep();
  }

  void Power_Class::timerSleep( const rtc_date_t& date, const rtc_time_t& time)
  {
    M5.Rtc.disableIRQ();
    M5.Rtc.setAlarmIRQ(date, time);
    _timerSleep();
  }

#if !defined (M5UNIFIED_PC_BUILD)

  static std::int32_t getBatteryAdcRaw(adc1_channel_t adc_ch)
  {
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3)
    static constexpr int BASE_VOLATAGE = 3600;

    static esp_adc_cal_characteristics_t* adc_chars = nullptr;
    if (adc_chars == nullptr)
    {
      adc1_config_width(ADC_WIDTH_BIT_12);
      adc1_config_channel_atten(adc_ch, ADC_ATTEN_DB_11);
      adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
      esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, BASE_VOLATAGE, adc_chars);
    }
    return esp_adc_cal_raw_to_voltage(adc1_get_raw(adc_ch), adc_chars);
#else
    return 0;
#endif
  }

#endif

  int16_t Power_Class::getBatteryVoltage(void)
  {
#if !defined (M5UNIFIED_PC_BUILD)
    switch (_pmic)
    {

#if defined (CONFIG_IDF_TARGET_ESP32C3)
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_ip5306:
      break;

    case pmic_t::pmic_axp192:
      return Axp192.getBatteryVoltage() * 1000;

#endif

    case pmic_t::pmic_axp2101:
      return Axp2101.getBatteryVoltage() * 1000;

#endif

    case pmic_t::pmic_adc:
      return getBatteryAdcRaw(_batAdc) * _adc_ratio;

    default:
      return 0;
    }
#endif
    return 0;
  }

  std::int32_t Power_Class::getBatteryLevel(void)
  {
#if defined (M5UNIFIED_PC_BUILD)
    return 100;
#else
    float mv = 0.0f;
    switch (_pmic)
    {

#if defined (CONFIG_IDF_TARGET_ESP32C3)
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_ip5306:
      return Ip5306.getBatteryLevel();

    case pmic_t::pmic_axp192:
      mv = Axp192.getBatteryVoltage() * 1000;
      break;

#endif

    case pmic_t::pmic_axp2101:
      return Axp2101.getBatteryLevel();
      break;

#endif

    case pmic_t::pmic_adc:
      mv = getBatteryAdcRaw(_batAdc) * _adc_ratio;
      break;

    default:
      return -2;
    }

    int level = (mv - 3300) * 100 / (float)(4150 - 3350);

    return (level < 0) ? 0
         : (level >= 100) ? 100
         : level;
#endif
  }

  void Power_Class::setBatteryCharge(bool enable)
  {
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_ip5306:
      Ip5306.setBatteryCharge(enable);
      return;

    case pmic_t::pmic_axp192:
      Axp192.setBatteryCharge(enable);
      return;

#endif

    case pmic_t::pmic_axp2101:
      Axp2101.setBatteryCharge(enable);
      break;

#endif

    default:
      return;
    }
  }

  void Power_Class::setChargeCurrent(std::uint16_t max_mA)
  {
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_ip5306:
      Ip5306.setChargeCurrent(max_mA);
      return;

    case pmic_t::pmic_axp192:
      Axp192.setChargeCurrent(max_mA);
      return;

#endif

    case pmic_t::pmic_axp2101:
      Axp2101.setChargeCurrent(max_mA);
      break;

#endif

    default:
      return;
    }
  }

  int32_t Power_Class::getBatteryCurrent(void)
  {
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#else

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_axp192:
      {
        int32_t res = Axp192.getBatteryChargeCurrent();
        int32_t dsc = Axp192.getBatteryDischargeCurrent();
        if (res < dsc) res = -dsc;
        return res;
      }
#endif

    case pmic_t::pmic_axp2101:

#if defined (CONFIG_IDF_TARGET_ESP32S3)
      // for CoreS3
      return 0;

#else

      // for Core2 v1.1
      return 1000.0f * Ina3221.getCurrent(0); // 0=CH1. CH1=BAT Current.

#endif

#endif

    default:
      return 0;
    }
  }

  void Power_Class::setChargeVoltage(std::uint16_t max_mV)
  {
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    case pmic_t::pmic_ip5306:
      Ip5306.setChargeVoltage(max_mV);
      return;

    case pmic_t::pmic_axp192:
      Axp192.setChargeVoltage(max_mV);
      return;

#endif

    case pmic_t::pmic_axp2101:
      Axp2101.setChargeVoltage(max_mV);
      break;

#endif

    default:
      return;
    }
  }

  Power_Class::is_charging_t Power_Class::isCharging(void)
  {
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    case pmic_t::pmic_ip5306:
      return Ip5306.isCharging() ? is_charging_t::is_charging : is_charging_t::is_discharging;

    case pmic_t::pmic_axp192:
      return Axp192.isCharging() ? is_charging_t::is_charging : is_charging_t::is_discharging;

#endif

    case pmic_t::pmic_axp2101:
      return Axp2101.isCharging() ? is_charging_t::is_charging : is_charging_t::is_discharging;
    //   return Axp2101.getChargeDirection() ? is_charging_t::is_charging : is_charging_t::is_discharging;

#endif

    default:
      return is_charging_t::charge_unknown;
    }
  }

  uint8_t Power_Class::getKeyState(void)
  {
    switch (_pmic)
    {

#if defined (CONFIG_IDF_TARGET_ESP32C3)
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    case pmic_t::pmic_axp192:
      return Axp192.getPekPress();

#endif

    case pmic_t::pmic_axp2101:
      return Axp2101.getPekPress();

#endif

    default:
      return 0;
    }
  }

  void Power_Class::setVibration(uint8_t level)
  {
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (M5.getBoard() == board_t::board_M5StackCore2)
    {
      uint32_t mv = level ? 480 + level * 12 : 0;
      switch (_pmic)
      {
        case pmic_t::pmic_axp192:
          Axp192.setLDO3(mv);
          break;

        case pmic_t::pmic_axp2101:
          Axp2101.setDLDO1(mv);
          break;

        default:
          break;
      }
    }
#endif
  }

}
