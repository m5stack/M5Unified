// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Power_Class.hpp"

#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <soc/adc_channel.h>
#include "../M5Unified.hpp"

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
  static constexpr int CoreInk_POWER_HOLD_PIN = 12;
  static constexpr int M5Paper_POWER_HOLD_PIN =  2;
  static constexpr int M5Paper_EXT5V_ENABLE_PIN = 5;

  bool Power_Class::begin(void)
  {
    _pmic = pmic_t::pmic_unknown;

    /// setup power management ic
    switch (M5.getBoard())
    {
    default:
      break;

    case board_t::board_M5StackCoreInk:
      _wakeupPin = GPIO_NUM_27; // power button;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 25.1f / 5.1f;
      break;

    case board_t::board_M5Paper:
      m5gfx::pinMode(M5Paper_EXT5V_ENABLE_PIN, m5gfx::pin_mode_t::output);
      _wakeupPin = GPIO_NUM_36; // touch panel INT;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 2.0f;
      break;

    case board_t::board_M5Tough:
    case board_t::board_M5StackCore2:
      _wakeupPin = GPIO_NUM_39; // touch panel INT;
      NON_BREAK;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
    case board_t::board_M5Station:
      _pmic = Power_Class::pmic_t::pmic_axp192;
      Axp192.begin();
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

    if (_pmic == Power_Class::pmic_t::pmic_axp192)
    {
      static constexpr std::uint8_t reg_data_array[] =
        ///       +--------- VBUS-IPSOUT channel selection control signal when VBUS is available (0:The N_VBUSEN pin determines whether to open this channel. / 1: The VBUS-IPSOUT path can be selected to be opened regardless of the status of N_VBUSEN)
        ///       |+-------- VBUS VHOLD pressure limit control (0:disable ; 1:enable)
        ///       ||+++----- VHOLD setting (x 100mV + 4.0V ;  000:4.0V ～ 111:4.7V)
        ///       |||||+---- reserved(0)
        ///       ||||||+--- VBUS current limit control enable signal
        ///       |||||||+-- VBUS current limit control opens time limit flow selection (0:500mA ; 1:100mA)
        ///       ||||||||
        { 0x30, 0b00000010 // reg30h VBUS-IPSOUT Pass-Through Management

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

        , 0x12, 0x07    // reg12h enable DCDC1,DCDC3,LDO2  /  disable LDO3,DCDC2,EXTEN
        , 0x26, 0x6A    // reg26h DCDC1 3350mV (ESP32 VDD)
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
        Axp192.writeRegister8(0x92, 0x00);  // GPIO1 NMOS OpenDrain (SystemLED)
        Axp192.setChargeCurrent(390); // Core2 battery = 390mAh
        break;

      case board_t::board_M5Tough:
        Axp192.setLDO2(3300); // LCD + SD peripheral
        Axp192.setDCDC3(0);
        break;

      case board_t::board_M5Station:
        {
          static constexpr std::uint8_t reg92h_95h[] = 
          { 0x00 // GPIO1 NMOS OpenDrain
          , 0x00 // GPIO2 NMOS OpenDrain
          , 0x00 // GPIO3 low, GPIO4 low
          , 0x05 // GPIO3 NMOS OpenDrain, GPIO4 NMOS OpenDrain
          };
          Axp192.writeRegister(0x92, reg92h_95h, sizeof(reg92h_95h));
        }
        break;

      default:
        break;
      }
    }

    return (_pmic != pmic_t::pmic_unknown);
  }

  void Power_Class::setExtPower(bool enable)
  {
    switch (M5.getBoard())
    {
    case board_t::board_M5Paper:
      if (enable) { m5gfx::gpio_hi(M5Paper_EXT5V_ENABLE_PIN); }
      else        { m5gfx::gpio_lo(M5Paper_EXT5V_ENABLE_PIN); }
      break;

    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
      Axp192.writeRegister8(0x90, enable ? 0x02 : 0x07); // GPIO0 : enable=LDO / disable=float
      NON_BREAK;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      Axp192.setEXTEN(enable);
      break;

    default:
      break;
    }
  }

  void Power_Class::_powerOff(bool withTimer)
  {
    switch (_pmic)
    {
    case pmic_t::pmic_adc:
      m5gfx::gpio_lo( M5.getBoard() == board_t::board_M5StackCoreInk
                    ? CoreInk_POWER_HOLD_PIN
                    : M5Paper_POWER_HOLD_PIN
                    );
      break;

    case pmic_t::pmic_axp192:
      Axp192.powerOff();
      break;
    
    case pmic_t::pmic_ip5306:
      Ip5306.setPowerBoostKeepOn(withTimer);
      break;

    case pmic_t::pmic_unknown:
    default:
      break;
    }
    esp_deep_sleep_start();
  }

  void Power_Class::_timerSleep(void)
  {
    M5.Display.sleep();
    M5.Display.waitDisplay();

    switch (M5.getBoard())
    {
    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      /// RTCタイマーは指定時間になるとGPIO35をLOWにすることで通知を行うが、
      /// 回路設計の問題でINTピン(GPIO35)がプルアップされておらず、そのままでは利用できない。;
      /// そのため、同じくGPIO35に接続されているMPU6886のINTピンを利用してプルアップを実施する。;
      /// IMUの種類がMPU6886でない個体は対応できない (SH200Qではできない);
      M5.Imu.Mpu6886.setINTPinActiveLogic(true);
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
      esp_deep_sleep_start();
      return;

    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
      // esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0); // タッチパネルINT;
      // Axp192.powerOff();
      break;

    default:
      break;
    }
    _powerOff(true);
  }

  void Power_Class::deepSleep(std::uint64_t micro_seconds, bool touch_wakeup)
  {
    M5.Display.sleep();
    ESP_LOGD("Power","deepSleep");
    if (_pmic == pmic_t::pmic_ip5306)
    {
      Ip5306.setPowerBoostKeepOn(true);
    }

    if (touch_wakeup && _wakeupPin >= 0)
    {
      esp_sleep_enable_ext0_wakeup((gpio_num_t)_wakeupPin, false);
      while (m5gfx::gpio_in(_wakeupPin) == false)
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
    esp_deep_sleep_start();
  }

  void Power_Class::lightSleep(std::uint64_t micro_seconds, bool touch_wakeup)
  {
    ESP_LOGD("Power","lightSleep");
    if (_pmic == pmic_t::pmic_ip5306)
    {
      Ip5306.setPowerBoostKeepOn(true);
    }

    if (touch_wakeup && _wakeupPin >= 0)
    {
      esp_sleep_enable_ext0_wakeup((gpio_num_t)_wakeupPin, false);
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);
      while (m5gfx::gpio_in(_wakeupPin) == false)
      {
        m5gfx::delay(10);
      }
    }
    if (micro_seconds > 0){
      esp_sleep_enable_timer_wakeup(micro_seconds);
    }else{
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }
    esp_light_sleep_start();
  }

  void Power_Class::powerOff(void)
  {
    M5.Display.sleep();
    M5.Display.waitDisplay();
    _powerOff(false);
  }

  void Power_Class::timerSleep( int seconds )
  {
    M5.Rtc.clearIRQ();
    M5.Rtc.setAlarmIRQ(seconds);
    esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
    _timerSleep();
  }

  void Power_Class::timerSleep( const rtc_time_t& time)
  {
    M5.Rtc.clearIRQ();
    M5.Rtc.setAlarmIRQ(time);
    _timerSleep();
  }

  void Power_Class::timerSleep( const rtc_date_t& date, const rtc_time_t& time)
  {
    M5.Rtc.clearIRQ();
    M5.Rtc.setAlarmIRQ(date, time);
    _timerSleep();
  }


  static std::int32_t getBatteryAdcRaw(void)
  {
    static constexpr int BASE_VOLATAGE = 3600;
    static constexpr auto CoreInk_M5Paper_BAT_ADC = ADC1_GPIO35_CHANNEL;

    static esp_adc_cal_characteristics_t* adc_chars = nullptr;
    if (adc_chars == nullptr)
    {
      adc1_config_width(ADC_WIDTH_BIT_12);
      adc1_config_channel_atten(CoreInk_M5Paper_BAT_ADC, ADC_ATTEN_DB_11);
      adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
      esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, BASE_VOLATAGE, adc_chars);
    }
    return esp_adc_cal_raw_to_voltage(adc1_get_raw(CoreInk_M5Paper_BAT_ADC), adc_chars);
  }

  std::int32_t Power_Class::getBatteryLevel(void)
  {
    float volt;
    switch (_pmic)
    {
    case pmic_t::pmic_ip5306:
      return Ip5306.getBatteryLevel();

    case pmic_t::pmic_axp192:
      volt = Axp192.getBatteryVoltage() * 1000;
      break;

    case pmic_t::pmic_adc:
      volt = getBatteryAdcRaw() * _adc_ratio;
      break;
    
    default:
      return -2;
    }

    volt = (volt - 3300) * 100 / (float)(4150 - 3350);

    return (volt < 0) ? 0
         : (volt >= 100) ? 100
         : volt;
  }

  void Power_Class::setBatteryCharge(bool enable)
  {
    switch (_pmic)
    {
    case pmic_t::pmic_ip5306:
      Ip5306.setBatteryCharge(enable);
      return;

    case pmic_t::pmic_axp192:
      Axp192.setBatteryCharge(enable);
      return;

    default:
      return;
    }
  }

  void Power_Class::setChargeCurrent(std::uint16_t max_mA)
  {
    switch (_pmic)
    {
    case pmic_t::pmic_ip5306:
      Ip5306.setChargeCurrent(max_mA);
      return;

    case pmic_t::pmic_axp192:
      Axp192.setChargeCurrent(max_mA);
      return;

    default:
      return;
    }
  }

  void Power_Class::setChargeVoltage(std::uint16_t max_mV)
  {
    switch (_pmic)
    {
    case pmic_t::pmic_ip5306:
      Ip5306.setChargeVoltage(max_mV);
      return;

    case pmic_t::pmic_axp192:
      Axp192.setChargeVoltage(max_mV);
      return;

    default:
      return;
    }
  }

  Power_Class::is_charging_t Power_Class::isCharging(void)
  {
    switch (_pmic)
    {
    case pmic_t::pmic_ip5306:
      return Ip5306.isCharging() ? is_charging_t::is_charging : is_charging_t::is_discharging;

    case pmic_t::pmic_axp192:
      return Axp192.isCharging() ? is_charging_t::is_charging : is_charging_t::is_discharging;

    default:
      return is_charging_t::charge_unknown;
    }
  }

}
