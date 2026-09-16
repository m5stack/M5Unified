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
#include "power/M5PM1_Class.hpp"
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
  , ext_EXT    = 1 << 8 // M5Tab5X bottom Hat power.
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

  /// Battery charge state.
  /// The sign carries the meaning: negative = the state could not be obtained,
  /// 0 = this model can never obtain it, positive = a state was obtained.
  /// @note The numeric values are fixed. They are burned into the caller's
  /// translation unit as immediates, so new values may only be appended and a
  /// removed value stays retired forever.
  enum class charge_state_t : std::int8_t
  { not_initialized = -3  ///< M5.begin() has not completed yet.
  , io_error        = -2  ///< the read procedure failed. (I2C NACK, charger gate timeout, ...)
  , undetermined    = -1  ///< the read succeeded, but the available signals do not decide a single state.
  , unsupported     =  0  ///< this model can never assert any positive state.
  , charging        =  1  ///< charging.
  , not_charging    =  2  ///< not charging. (nothing more can be said)
  , full            =  3  ///< the charger reports the charge as complete.
  , disabled        =  4  ///< charging is disabled.
  , discharging     =  5  ///< not charging, and the battery is being drained.
  , idle            =  6  ///< not charging, and nothing flows in or out of the battery.
  };
  /// The highest defined state. Bump it when a state is appended, so that the
  /// set type keeps rejecting values above it.
  constexpr charge_state_t charge_state_max = charge_state_t::idle;

  class charge_state_set_t;
  constexpr charge_state_set_t operator|(charge_state_t a, charge_state_t b);
  constexpr charge_state_set_t operator|(charge_state_set_t s, charge_state_t v);

  /// A set of charge states.
  /// @note Negative values and 0 are members of no set and match no set, on the
  /// construction side as well as on the query side. Out of range values are
  /// ignored the same way. Carrying that guarantee in the type is the reason
  /// this class exists: written by hand as "(int)state & mask", io_error would
  /// match almost every mask and a failed read would be reported as
  /// "not charging".
  class charge_state_set_t
  {
  public:
    typedef std::uint32_t storage_type;

    constexpr charge_state_set_t(void) : _bits { 0 } {}

    /// @return true if the state is a member of this set. A non positive state is never a member.
    constexpr bool contains(charge_state_t state) const { return (_bits & _bit_of(state)) != 0; }

    /// @return true if at least one state is a member of both sets.
    constexpr bool intersects(charge_state_set_t other) const { return (_bits & other._bits) != 0; }

    /// @return true if this set holds no state at all.
    constexpr bool empty(void) const { return _bits == 0; }

  private:
    constexpr explicit charge_state_set_t(storage_type bits) : _bits { bits } {}

    /// Range checked bit position. Only the defined positive states map to a
    /// bit: negative values, 0 and anything above charge_state_max map to no
    /// bit at all, so they can neither be added to a set nor match one. The
    /// check comes before the shift (a negative or too wide shift is
    /// undefined behaviour).
    static constexpr storage_type _bit_of(charge_state_t state)
    {
      return ((std::int8_t)state > 0 && (std::int8_t)state <= (std::int8_t)charge_state_max)
           ? (storage_type)1 << (std::int8_t)state
           : (storage_type)0;
    }

    storage_type _bits;

    friend constexpr charge_state_set_t operator|(charge_state_t a, charge_state_t b);
    friend constexpr charge_state_set_t operator|(charge_state_set_t s, charge_state_t v);
  };

  constexpr charge_state_set_t operator|(charge_state_t a, charge_state_t b)
  { return charge_state_set_t(charge_state_set_t::_bit_of(a) | charge_state_set_t::_bit_of(b)); }

  constexpr charge_state_set_t operator|(charge_state_set_t s, charge_state_t v)
  { return charge_state_set_t(s._bits | charge_state_set_t::_bit_of(v)); }

  /// The highest state must stay inside the storage of the set type.
  static_assert((int)charge_state_max < (int)(sizeof(charge_state_set_t::storage_type) * 8)
              , "charge_state_t no longer fits in charge_state_set_t::storage_type");

  /// Every positive state. Use it to ask whether a state was obtained at all.
  constexpr charge_state_set_t charge_states_known
    = charge_state_t::charging    | charge_state_t::not_charging
    | charge_state_t::full        | charge_state_t::disabled
    | charge_state_t::discharging | charge_state_t::idle;

  /// The states that mean "charging". (a future trickle state would be added here)
  constexpr charge_state_set_t charge_states_any_charging
    = charge_state_set_t() | charge_state_t::charging;

  /// The states that mean "not charging".
  /// full belongs here: it only claims that the charger reports completion, not
  /// that nothing is being drawn from the battery.
  constexpr charge_state_set_t charge_states_any_not_charging
    = charge_state_t::not_charging | charge_state_t::full     | charge_state_t::disabled
    | charge_state_t::discharging  | charge_state_t::idle;

  /// Whether a battery is attached.
  /// The sign rule is the same as charge_state_t.
  enum class battery_presence_t : std::int8_t
  { not_initialized = -3  ///< M5.begin() has not completed yet.
  , io_error        = -2  ///< the read procedure failed.
  , undetermined    = -1  ///< the read succeeded, but the presence is not decided yet.
  , unsupported     =  0  ///< this model can never tell.
  , absent          =  1  ///< no battery is attached.
  , present         =  2  ///< a battery is attached.
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
    , pmic_m5pm1
    };

    /// @deprecated No function returns this any more; it will be removed in the
    /// next release. Use charge_state_t. (no attribute is attached on purpose)
    enum is_charging_t
    { is_discharging = 0
    , is_charging
    , charge_unknown
    };

    /// The charge control paths a model provides. (see getChargeControlCaps)
    /// @note This mask is deliberately separate from the state set, so that a
    /// state value never gets tied to a bit position.
    enum charge_control_capability_t : std::uint8_t
    { cap_set_charge_enable  = 1u << 0
    , cap_set_charge_current = 1u << 1
    , cap_set_charge_voltage = 1u << 2
    };

    bool begin(void);

    /// Set power output of the external ports.
    /// @param enable true=output / false=input
    /// @param port_mask for M5Station. ext_port (bitmask).
    /// @note On the CoreS3 family (CoreS3 / CoreS3 SE / StackChan), disabling an enabled output blocks for
    ///       about 200 ms (the boost converter is stopped first and the bus is left to discharge before the
    ///       switch-over), and enabling without a battery may block for up to 1 s while the protection check
    ///       waits for the TS reading to settle. The switch-over is serialized with setUsbOutput and the
    ///       internal speaker enable, so those may wait for it as well.
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

    /// Value for micro_seconds of deepSleep / lightSleep, meaning "sleep without a timer wakeup".
    /// The device sleeps until a wakeup pin or another wakeup source is triggered.
    static constexpr std::uint64_t sleep_no_timer = ~0ull;

    /// ESP32 deepsleep
    /// @param micro_seconds Number of micro seconds to wakeup. 0 = do not sleep. sleep_no_timer = no timer wakeup.
    /// @param touch_wakeup Enable wakeup by the wakeup pin of the device, if it has one.
    /// @attention Waking up from deep sleep restarts the program from the beginning.
    void deepSleep(std::uint64_t micro_seconds = sleep_no_timer, bool touch_wakeup = true);

    /// ESP32 lightsleep
    /// @param micro_seconds Number of micro seconds to wakeup. 0 = do not sleep. sleep_no_timer = no timer wakeup.
    /// @param touch_wakeup Enable wakeup by the wakeup pin of the device, if it has one.
    void lightSleep(std::uint64_t micro_seconds = sleep_no_timer, bool touch_wakeup = true);

    /// Get the remaining battery power.
    /// @return 0-100 level
    std::int32_t getBatteryLevel(void);

    /// set battery charge enable.
    /// @param enable true=enable / false=disable
    /// @return true if the path exists and the write succeeded.
    /// @note false only means that the requested setting did not take effect;
    /// it says nothing about what the hardware currently holds.
    bool setBatteryCharge(bool enable);

    /// set battery charge current
    /// @param max_mA milli ampere.
    /// @param applied_mA optional. receives the step that was applied.
    /// @return true if the path exists and the write succeeded.
    /// @note The highest step not exceeding max_mA is selected; a request below
    /// the lowest step is clamped up to it instead of being rejected. 0 is not a
    /// step: use setBatteryCharge(false) to stop charging.
    /// @note applied_mA is left untouched when false is returned.
    /// @note CoreMatrix selects 180 mA below 650 mA, otherwise 650 mA.
    /// @note ToughC5 selects 180 mA below 830 mA, otherwise 830 mA.
    /// @note 0 is not a step: where a current path exists it selects the
    /// lowest one, and a warning is logged once. Use setBatteryCharge(false)
    /// to stop charging.
    /// @note Tab5 / Tab5X select 500 mA below 1000 mA, otherwise 1000 mA.
    /// @note StampS3Bat selects 200 mA below 650 mA, otherwise 650 mA.
    /// @attention Returns false on models without a current control path; see
    /// getChargeControlCaps(). The M5Stack with a non-I2C IP5306 also returns false.
    bool setChargeCurrent(std::uint16_t max_mA, std::uint16_t* applied_mA = nullptr);

    /// set battery charge voltage
    /// @param max_mV milli volt.
    /// @param applied_mV optional. receives the step that was applied.
    /// @return true if the path exists and the write succeeded.
    /// @note The step selection rule is the same as setChargeCurrent.
    /// @note applied_mV is left untouched when false is returned.
    /// @attention Returns false on models without a voltage control path; see
    /// getChargeControlCaps(). The M5Stack with a non-I2C IP5306 also returns false.
    bool setChargeVoltage(std::uint16_t max_mV, std::uint16_t* applied_mV = nullptr);

    /// Get which charge control paths this model provides.
    /// @return bitmask of charge_control_capability_t. 0 = no control path,
    /// which is also what is returned before M5.begin() has completed.
    /// @note Only the set side is declared here; there is no readback getter
    /// yet, so no readback capability is advertised.
    std::uint8_t getChargeControlCaps(void);

    /// Get the battery charge state.
    /// @return a single charge_state_t value. Never a set.
    /// @note The idiom is a set test on both sides:
    /// @code
    ///   auto s = M5.Power.getChargeState();
    ///   if      (charge_states_any_charging.contains(s))     { /* charging */ }
    ///   else if (charge_states_any_not_charging.contains(s)) { /* not charging (how detailed depends on the model) */ }
    ///   else                                                 { /* could not be obtained */ }
    /// @endcode
    /// Comparing against a single value ( s == charge_state_t::not_charging )
    /// is model dependent and breaks silently when a model learns to report a
    /// more detailed state, so both branches use a set.
    /// @attention The last branch merges four different reasons:
    /// not_initialized (called too early) / io_error (not readable right now) /
    /// undetermined (not enough evidence) / unsupported (model can never tell).
    /// Look at the value itself to tell them apart.
    charge_state_t getChargeState(void);

    /// Get the set of states this model can report.
    /// @param caps output parameter, receives the set of reportable states.
    /// @return false before M5.begin() has completed. caps is left untouched then.
    /// @note An empty set after M5.begin() means getChargeState() always answers
    /// unsupported. The set is decided once during M5.begin() and does not
    /// change afterwards: a communication failure is reported as io_error and
    /// never shrinks the set. The one exception is a board that carries either
    /// an AXP192 or an AXP2101 whose chip-ID probes all failed during
    /// M5.begin(): it runs with the board default (AXP192) until an explicit
    /// M5.Power.begin() gets a positive ID, which can widen the set.
    bool getChargeStateCaps(charge_state_set_t* caps);

    /// @return true if the caps could be obtained and they contain the state.
    bool canReport(charge_state_t state);

    /// Get whether the battery is currently charging or not.
    /// @return true only while charging. Everything else - full, disabled,
    /// unknown, and a failed read - returns false.
    /// @note This is charge_states_any_charging.contains(getChargeState()).
    /// @attention Always false on models that cannot report charging; see
    /// canReport(charge_state_t::charging). The M5Stack with a non-I2C IP5306
    /// also returns false (io_error).
    bool isCharging(void);

    /// Get whether a battery is attached.
    /// @note The existing sentinels of getBatteryVoltage() (0 / -1) are kept
    /// unchanged for compatibility; this is the typed way to ask.
    battery_presence_t getBatteryPresence(void);

    /// Get VBUS voltage
    /// @return VBUS voltage [mV] / -1=not supported model
    /// @attention Only for models with AXP192, AXP2101, or M5PM1 VBUS monitoring
    int16_t getVBUSVoltage(void);

    /// Get battery voltage
    /// @return battery voltage [mV]
    /// @attention Models with battery detection ( ex. CoreMatrix , ToughC5 )
    /// return 0 when no battery is attached and -1 while the presence has
    /// not been determined yet (shortly after boot).
    int16_t getBatteryVoltage(void);

    /// get battery current
    /// @return battery current [mA] ( +=charge / -=discharge )
    /// @attention This reading comes from the hardware of the board: an AXP192, or a
    /// dedicated current sense IC ( ex. Core2 v1.1 , M5Tab5 , M5PowerHub ).
    /// Boards without either of them return 0.
    int32_t getBatteryCurrent(void);

    /// Get Ext Port voltage
    /// @return Ext voltage [mV]
    float getExtVoltage(ext_port_mask_t port_mask);

    /// get Ext Port current
    /// @return Ext current [mA] ( +=charge / -=discharge )
    float getExtCurrent(ext_port_mask_t port_mask);

    /// Get Power Key Press condition.
    /// @return 0=none / 1=long pressed / 2=short clicked / 3=both
    /// @attention Only for models with AXP192, AXP2101, or M5PM1.
    /// @attention M5PM1 reports only 0 or 2.
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
    M5PM1_Class M5pm1;
    INA226_Class Ina226 = { 0x40 };

#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)

    AW32001_Class Aw32001;
    BQ27220_Class Bq27220;

#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    M5PM1_Class M5pm1;

#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    M5PM1_Class M5pm1;
    INA226_Class Ina226 = { 0x41 };

#else

    AXP2101_Class Axp2101;
    AXP192_Class Axp192;
    IP5306_Class Ip5306;
    // secondery INA3221 for M5Station.
    INA3221_Class Ina3221[2] = { { 0x40 }, { 0x41 } };

#if defined (CONFIG_IDF_TARGET_ESP32C5)
    M5PM1_Class M5pm1;
#endif

#endif

  private:
    /// CoreS3 family: AW9523 のビット操作を setExtOutput / setUsbOutput と同じ排他区間で行う (スピーカー制御用)
    static void _core_s3_aw9523_bit(uint8_t reg, uint8_t mask, bool on);
    /// Battery current with I2C error reporting, for the boards whose charge
    /// state is decided by the current. @return false = not readable / no path.
    bool _readBatteryCurrent(std::int32_t* mA);
    /// The state procedure itself. getChargeState() wraps it with the check
    /// that the answer is one of the advertised capabilities.
    charge_state_t _getChargeState(void);
    std::int32_t _getBatteryAdcRaw(void);
    void _powerOff(bool withTimer);
    void _timerSleep(void);

#if defined (CONFIG_IDF_TARGET_ESP32C5) || defined (CONFIG_IDF_TARGET_ESP32C61)
    /// Check whether a battery is actually attached (non-blocking).
    /// @param io_ok optional. receives false if any read of this evaluation failed.
    /// @return 1=present / 0=absent / -1=not yet determined (cached across failed reads)
    std::int8_t _batteryPresent(bool* io_ok = nullptr);
    /// Drop the transient presence evidence (streak, sample baseline, counters); the verdict is kept.
    void _bp_dropEvidence(void);
    /// Read the raw charger CHG_STAT line. @return false=not readable
    bool _readChargeStat(bool* level);
    /// Whether the VBAT node is confirmed collapsed (false when unreadable).
    /// @param io_ok optional. receives false when the node state could not be read.
    bool _vbatNodeDown(bool* io_ok = nullptr);
    /// CHG_STAT behind the battery presence gate. (ToughC5 / CoreMatrix)
    charge_state_t _chargeStateFromChgStat(void);
    /// Battery presence. -1 = not yet determined.
    std::int8_t _batt_present = -1;
    /// Tick when charging last stopped (0 = at reset, which clears PWR_CFG).
    std::uint32_t _chg_off_ms = 0;
    /// Presence sampling state: last VBAT sample, CHG_STAT low since,
    /// and evidence counters. 0 in the tick fields = no sample yet.
    std::uint16_t _bp_last_mv = 0;
    std::uint32_t _bp_last_ms = 0;
    std::uint32_t _bp_chg_low_ms = 0;
    std::uint8_t _bp_stable = 0;
    std::uint8_t _bp_unstable = 0;
    std::uint8_t _bp_low = 0;
#endif

    /// Release the wakeup pin so that it can be asserted again while sleeping.
    /// @return true if the pin is released ( high ).
    bool _releaseWakeupPin(std::uint_fast8_t wakeup_pin, bool* clear_comm_ok = nullptr);
    float _readExtValue(ext_port_mask_t port_mask, bool is_voltage);

    float _adc_ratio = 0;
    /// true once M5Unified::begin() has completed (set there, not in
    /// Power_Class::begin()). _pmic alone cannot tell a pmic_unknown model
    /// from a not yet initialized one.
    bool _initialized = false;
    bool _identity_settled = false;   ///< a probe answered with a positive chip ID (AXP192 / AXP2101 boards)
    bool _identity_unconfirmed = false;   ///< every chip-ID probe failed: the board default is provisional (see begin())
    std::uint8_t _wakeupPin = 255;
    std::uint8_t _rtcIntPin = 255;
    pmic_t _pmic = pmic_t::pmic_unknown;
#if !defined (M5UNIFIED_PC_BUILD)
    uint8_t _batAdcCh;
    uint8_t _batAdcUnit;
    uint8_t _batAdcPin = 255;
#endif
  };
}

#endif
