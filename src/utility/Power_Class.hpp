// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_Power_Class_H__
#define __M5_Power_Class_H__

#include "m5unified_common.h"

#include "I2C_Class.hpp"
#include "power/AXP192_Class.hpp"
#include "power/AXP2101_Class.hpp"
#include "power/IP5306_Class.hpp"
#include "power/INA3221_Class.hpp"
#include "power/INA226_Class.hpp"
#include "power/AW32001_Class.hpp"
#include "power/BQ27220_Class.hpp"
#include "RTC_Class.hpp"

#if __has_include (<sdkconfig.h>)
#include <sdkconfig.h>
#endif

#if __has_include (<esp_adc/adc_oneshot.h>) // ESP-IDF v5 or later
#include <esp_adc/adc_oneshot.h>
 #if __has_include(<esp_adc/adc_cali.h>)
  #include <esp_adc/adc_cali.h>
 #endif
#elif __has_include (<driver/adc.h>)
#include <driver/adc.h>
#include <esp_adc_cal.h>
#endif

namespace m5
{
  class M5Unified;

  enum ext_port_mask_t
  { ext_none = 0
  // For individual control of external ports of M5Station and M5PowerHub.
  , ext_PA     = 1 << 0
  , ext_PB1    = 1 << 1
  , ext_PB2    = 1 << 2
  , ext_PC1    = 1 << 3
  , ext_PC2    = 1 << 4
  , ext_USB    = 1 << 5 // M5Station external USB.   ※ Not for CoreS3 main USB.
  , ext_PWR485 = 1 << 6 // M5PowerHub external RS485.
  , ext_PWRCAN = 1 << 7 // M5PowerHub external CAN.
  , ext_MAIN   = 1 << 15
  };

  struct ext_port_bus_t
  {
    // output voltage of the external port(3000~20000mV, step 20mV).
    uint16_t voltage;

    // output current limit(0~232mA).
    uint8_t currentLimit;

    // output enable/disable.
    bool enable = 0;

    // output direction. true=output / false=input
    bool direction = 0;
  };

  class Power_Class
  {
  friend M5Unified;
  public:

    enum pmic_t
    { pmic_unknown
    , pmic_adc
    , pmic_axp192
    , pmic_ip5306
    , pmic_axp2101
    , pmic_aw32001
    };

    enum is_charging_t
    { is_discharging = 0
    , is_charging
    , charge_unknown
    };

    bool begin(void);

    /// Set power output of the external ports.
    /// @param enable true=output / false=input
    /// @param port_mask for M5Station. ext_port (bitmask).
    void setExtOutput(bool enable, ext_port_mask_t port_mask = (ext_port_mask_t)0xFF);

    /// deprecated : Change to "setExtOutput"
    [[deprecated("Change to setExtOutput")]]
    void setExtPower(bool enable, ext_port_mask_t port_mask = (ext_port_mask_t)0xFF) { setExtOutput(enable, port_mask); }

    /// Get power output of the external ports.
    /// @return true=output enabled / false=output disabled
    bool getExtOutput(void);

    /// Set power output of the main USB port.
    /// @param enable true=output / false=input
    /// @attention for M5Stack CoreS3 main USB port.
    /// @attention ※ Not for M5Station/M5Tab external USB.
    void setUsbOutput(bool enable);

    /// Get power output of the main USB port.
    /// @return true=output enabled / false=output disabled
    /// @attention for M5Stack CoreS3 main USB port.
    /// @attention ※ Not for M5Station/M5Tab external USB.
    bool getUsbOutput(void);

    /// Turn on/off the power LED.
    /// @param brightness 0=OFF: 1~255=ON (Set brightness if possible.)
    void setLed(uint8_t brightness = 255);

    /// all power off.
    void powerOff(void);

    /// sleep and timer boot. The boot condition can be specified by the argument.
    /// @param seconds Number of seconds to boot.
    void timerSleep(int seconds);

    /// sleep and timer boot. The boot condition can be specified by the argument.
    /// @param time Time to boot. (only minutes and hours can be specified. Ignore seconds)
    /// @attention CoreInk and M5Paper can't alarm boot because it can't be turned off while connected to USB.
    /// @attention CoreInk と M5Paper は USB接続中はRTCタイマー起動が出来ない。;
    void timerSleep(const rtc_time_t& time);

    /// sleep and timer boot. The boot condition can be specified by the argument.
    /// @param date Date to boot. (only date and weekDay can be specified. Ignore year and month)
    /// @param time Time to boot. (only minutes and hours can be specified. Ignore seconds)
    /// @attention CoreInk and M5Paper can't alarm boot because it can't be turned off while connected to USB.
    /// @attention CoreInk と M5Paper は USB接続中はRTCタイマー起動が出来ない。;
    void timerSleep(const rtc_date_t& date, const rtc_time_t& time);

    /// ESP32 deepsleep
    /// @param seconds Number of micro seconds to wakeup.
    void deepSleep(std::uint64_t micro_seconds = 0, bool touch_wakeup = true);

    /// ESP32 lightsleep
    /// @param seconds Number of micro seconds to wakeup.
    void lightSleep(std::uint64_t micro_seconds = 0, bool touch_wakeup = true);

    /// Get the remaining battery power.
    /// @return 0-100 level
    std::int32_t getBatteryLevel(void);

    /// set battery charge enable.
    /// @param enable true=enable / false=disable
    void setBatteryCharge(bool enable);

    /// set battery charge current
    /// @param max_mA milli ampere.
    /// @attention Non-functioning models : CoreInk , M5Paper , M5Stack(with non I2C IP5306)
    void setChargeCurrent(std::uint16_t max_mA);

    /// set battery charge voltage
    /// @param max_mV milli volt.
    /// @attention Non-functioning models : CoreInk , M5Paper , M5Stack(with non I2C IP5306)
    void setChargeVoltage(std::uint16_t max_mV);

    /// Get whether the battery is currently charging or not.
    /// @attention Non-functioning models : CoreInk , M5Paper , M5Stack(with non I2C IP5306)
    is_charging_t isCharging(void);

    /// Get VBUS voltage
    /// @return VBUS voltage [mV] / -1=not supported model
    /// @attention Only for models with AXP192 or AXP2101
    int16_t getVBUSVoltage(void);

    /// Get battery voltage
    /// @return battery voltage [mV]
    int16_t getBatteryVoltage(void);

    /// get battery current
    /// @return battery current [mA] ( +=charge / -=discharge )
    int32_t getBatteryCurrent(void);

    /// Get Ext Port voltage
    /// @return Ext voltage [mV]
    int16_t getExtVoltage(ext_port_mask_t port_mask);

    /// get Ext Port current
    /// @return Ext current [mA] ( +=charge / -=discharge )
    int16_t getExtCurrent(ext_port_mask_t port_mask);

    /// Get Power Key Press condition.
    /// @return 0=none / 1=long pressed / 2=short clicked / 3=both
    /// @attention Only for models with AXP192 or AXP2101
    /// @attention Once this function is called, the value is reset to 0, and the next time it is pressed on, the value changes.
    uint8_t getKeyState(void);

    /// Set the configuration of the external port bus.
    /// @param config Configuration of the external port bus.
    /// @attention for M5PowerHub.
    void setExtPortBusConfig(const ext_port_bus_t& config);

    /// Operate the vibration motor
    /// @param level Vibration strength of the motor. (0=stop)
    void setVibration(uint8_t level);

    pmic_t getType(void) const { return _pmic; }

#if defined (CONFIG_IDF_TARGET_ESP32S3)

    AXP2101_Class Axp2101;

#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)

    AW32001_Class Aw32001;
    BQ27220_Class Bq27220;

#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    INA226_Class Ina226 = { 0x41 };

#else

    AXP2101_Class Axp2101;
    AXP192_Class Axp192;
    IP5306_Class Ip5306;
    // secondery INA3221 for M5Station.
    INA3221_Class Ina3221[2] = { { 0x40 }, { 0x41 } };

#endif

  private:
    std::int32_t _getBatteryAdcRaw(void);
    void _powerOff(bool withTimer);
    void _timerSleep(void);
    int16_t _readExtValue(ext_port_mask_t port_mask, int reg_offset);

    float _adc_ratio = 0;
    std::uint8_t _wakeupPin = 255;
    std::uint8_t _rtcIntPin = 255;
    pmic_t _pmic = pmic_t::pmic_unknown;
#if !defined (M5UNIFIED_PC_BUILD)
    uint8_t _batAdcCh;
    uint8_t _batAdcUnit;
#endif
  };
}

#endif
