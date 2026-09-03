// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "../M5Unified.hpp"
#include "Power_Class.hpp"
#include "M5IOE1_Class.hpp"

#if defined (M5UNIFIED_PC_BUILD) || defined (M5UNIFIED_CHECK_CHARGE_STATE_CAPS)
#include <cassert>
#endif

#if !defined (M5UNIFIED_PC_BUILD)

#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <soc/soc_caps.h>

// ESP-IDF v4 defines only the generic SOC_PM_SUPPORT_EXT_WAKEUP; the split into
// SOC_PM_SUPPORT_EXT0_WAKEUP / SOC_PM_SUPPORT_EXT1_WAKEUP came later.
// Without these fallbacks both wakeup branches below vanish when building with such an
// ESP-IDF, and the wakeup pin is then silently ignored on every board.
#if defined (SOC_PM_SUPPORT_EXT0_WAKEUP)
 #define M5UNIFIED_PM_SUPPORT_EXT0 SOC_PM_SUPPORT_EXT0_WAKEUP
#elif defined (SOC_PM_SUPPORT_EXT_WAKEUP) \
   && (defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S2) || defined (CONFIG_IDF_TARGET_ESP32S3))
 #define M5UNIFIED_PM_SUPPORT_EXT0 1
#else
 #define M5UNIFIED_PM_SUPPORT_EXT0 0
#endif

#if defined (SOC_PM_SUPPORT_EXT1_WAKEUP)
 #define M5UNIFIED_PM_SUPPORT_EXT1 SOC_PM_SUPPORT_EXT1_WAKEUP
#elif defined (SOC_PM_SUPPORT_EXT_WAKEUP)
 #define M5UNIFIED_PM_SUPPORT_EXT1 1
#else
 #define M5UNIFIED_PM_SUPPORT_EXT1 0
#endif
#include <soc/adc_channel.h>

// On Arduino builds the core owns the ADC driver (analogRead). Creating a
// separate adc_oneshot unit for the battery ADC makes both owners fight over
// the same unit and one side permanently reads 0, so the battery ADC must be
// read through the Arduino API instead. (analogReadMilliVolts: core v2.0.0+)
#if defined (ARDUINO) && __has_include (<esp_arduino_version.h>)
 #include <esp_arduino_version.h>
 #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 0)
  #include <esp32-hal-adc.h>
  #define M5UNIFIED_BATADC_USE_ARDUINO
 #endif
#endif

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

#if !defined (M5UNIFIED_PC_BUILD)
#if defined (CONFIG_IDF_TARGET_ESP32S3)
  static constexpr uint8_t aw9523_i2c_addr = 0x58;
  static constexpr uint8_t powerhub_i2c_addr = 0x50;
  static constexpr uint8_t ip2315_i2c_addr = 0x75; // M5PaperMono USB fast-charger
  static constexpr int M5PaperS3_CHG_STAT_PIN = GPIO_NUM_4;

  static void init_papermono_ip2315_access(void)
  {
    auto& ioe1 = M5.getIOExpander(0);
    ioe1.setHighImpedance(M5IOE1_Class::gpio11, false);
    ioe1.setDirection(M5IOE1_Class::gpio11, true);
    ioe1.digitalWrite(M5IOE1_Class::gpio11, false);
  }

  static bool set_papermono_ip2315_enabled(bool enable)
  {
    return M5.getIOExpander(0).digitalWrite(M5IOE1_Class::gpio11, enable);
  }

  static bool wait_papermono_ip2315_ready(void)
  {
    m5gfx::delay(2);
    for (int i = 0; i < 64; ++i)
    {
      if (M5.In_I2C.scanID(ip2315_i2c_addr, i2c_freq)) { return true; }
    }
    return false;
  }

#elif defined (CONFIG_IDF_TARGET_ESP32C6)
  static constexpr int M5NanoC6_LED_PIN = GPIO_NUM_7;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
  static constexpr int TimerCam_POWER_HOLD_PIN = GPIO_NUM_33;
  static constexpr int TimerCam_LED_PIN = GPIO_NUM_2;
  static constexpr int M5Paper_EXT5V_ENABLE_PIN = GPIO_NUM_5;
  static constexpr int StickCPlus2_LED_PIN = GPIO_NUM_19;
#endif
#endif

  bool Power_Class::begin(void)
  {
    /// On the boards that carry either an AXP192 or an AXP2101 the identity
    /// is settled by a positive chip ID from the probe, and a later call
    /// (M5.Power.begin() is public) then re-applies the register setup but
    /// keeps the identity, so the capability set never changes afterwards.
    /// If every probe failed, the board default (AXP192) is used but is not
    /// treated as settled: a later begin() probes again instead of freezing
    /// an identity that was never confirmed. Boards whose PMIC follows from
    /// the board id alone do not go through the probe.
    const bool identity_settled = _identity_settled;
    const pmic_t settled_pmic = _pmic;
    (void)identity_settled; (void)settled_pmic;   // unused on chips without the AXP192/AXP2101 probe
    _pmic = pmic_t::pmic_unknown;

#if !defined (M5UNIFIED_PC_BUILD)
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    /// setup power management ic
    switch (M5.getBoard())
    {
    default:
      break;

    case board_t::board_M5CoreP4X:
      {
        _pmic = pmic_t::pmic_m5pm1;
        M5pm1.begin();

        auto& ioe1 = M5.getIOExpander(0);
        // M5IOE1_G12 supplies the shared 3V3 rail for MBUS, TF card and sensors.
        ioe1.setHighImpedance(M5IOE1_Class::gpio12, false);
        ioe1.setDirection(M5IOE1_Class::gpio12, true);
        ioe1.digitalWrite(M5IOE1_Class::gpio12, true);

        // M5IOE1_G6 is the active-low charger status input.
        ioe1.setDirection(M5IOE1_Class::gpio6, false);
        ioe1.setPullMode(M5IOE1_Class::gpio6, IOExpander_Base::pull_up);
      }
      break;

    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
      {
        static constexpr std::uint8_t reg_array_0x43[] =
        { ///     +--------- HP_DET : Headphone detect
          ///     |+-------- CAM_RST : Camera reset
          ///     ||+------- TP_RST : Touch reset
          ///     |||+------ LCD_RST : LCD reset
          ///     ||||+----- NC
          ///     |||||+---- EXT5V_EN : Ext5V enable
          ///     ||||||+--- SPK_EN : Speaker enable
          ///     |||||||+-- RF_PTH_L_INT_H_EXT : antenna  L=internal / H=external
          ///     ||||||||
          0x05, 0b01110000,   // OUT_SET
          0x03, 0b01110011,   // IO_DIR
          0x07, 0b00001000,   // OUT_H_IM
          0x0D, 0b00000100,   // PULL_SEL
          0x0B, 0b00000100,   // PULL_EN
        };
        static constexpr std::uint8_t reg_array_0x44[] =
        { ///     +--------- CHG_EN
          ///     |+-------- CHG_STAT
          ///     ||+------- nCHG_QC_EN
          ///     |||+------ PWROFF_PLUSE
          ///     ||||+----- USB5V_EN
          ///     |||||+---- NC
          ///     ||||||+--- NC
          ///     |||||||+-- WLAN_PWR_EN
          ///     ||||||||
          0x05, 0b10000001,   // OUT_SET
          0x03, 0b10110001,   // IO_DIR
          0x07, 0b00000110,   // OUT_H_IM
          0x0D, 0b00001000,   // PULL_SEL
          0x0B, 0b00001000,   // PULL_EN
        };
        M5.getIOExpander(0).writeRegister8Array(reg_array_0x43, sizeof(reg_array_0x43));
        M5.getIOExpander(1).writeRegister8Array(reg_array_0x44, sizeof(reg_array_0x44));
        if (M5.getBoard() == board_t::board_M5Tab5X)
        {
          auto& ioe = M5.getIOExpander(0); // PI4IOE 0x43, ADDR grounded, bottom Hat power
          ioe.setHighImpedance(3, false);
          ioe.setDirection(3, true);
          ioe.digitalWrite(3, true);
        }
        Ina226.begin();
        INA226_Class::config_t cfg;
        cfg.sampling_rate = INA226_Class::Sampling::Rate16;
        cfg.bus_conversion_time = INA226_Class::ConversionTime::US_1100;
        cfg.shunt_conversion_time = INA226_Class::ConversionTime::US_1100;
        cfg.mode = INA226_Class::Mode::ShuntAndBus;
        cfg.shunt_res = 0.005f;
        cfg.max_expected_current = 2.0f;
        Ina226.config(cfg);
      }
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C6)

    // PI4IO E0
    //  P0 (UnitC6L + NessoN1:BTN1)
    //  P1 (UnitC6L:NC / NessoN1:BTN2)
    //  P2-P5 NC
    //  P5 LNA Enable
    //  P6 RF Switch
    //  P7 LoRa Reset
    // for LoraC6 internal IOEXP
    static constexpr const uint8_t reg_data_array_for_lorac6[] = {
      0x03, 0b11100000,   // PI4IO_REG_IO_DIR
      0x05, 0b10000000,   // PI4IO_REG_OUT_SET
      0x07, 0b00011100,   // PI4IO_REG_OUT_H_IM
      0x0D, 0b11000011,   // PI4IO_REG_PULL_SEL
      0x0B, 0b11000011,   // PI4IO_REG_PULL_EN
      0x09, 0b00000011,   // PI4IO_REG_IN_DEF_STA
      0x11, 0b11111100,   // PI4IO_REG_INT_MASK
    };

    switch (M5.getBoard())
    {
    default:
      break;

    case board_t::board_M5UnitC6L:
      M5.getIOExpander(0).writeRegister8Array(reg_data_array_for_lorac6, sizeof(reg_data_array_for_lorac6));
      break;

    case board_t::board_ArduinoNessoN1:
      M5.getIOExpander(0).writeRegister8Array(reg_data_array_for_lorac6, sizeof(reg_data_array_for_lorac6));
      _pmic = pmic_t::pmic_aw32001;
      Aw32001.begin();
      Aw32001.setBatteryCharge(true);
      Aw32001.setChargeCurrent(100);
      Aw32001.setChargeVoltage(4200);

      Bq27220.begin();
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C61)

    /// setup power management ic
    switch (M5.getBoard())
    {
    default:
      break;

    case board_t::board_M5CoreMatrix:
      _pmic = pmic_t::pmic_m5pm1;
      _wakeupPin = GPIO_NUM_2;
      /// bring up the PM1 early so its status registers are readable below.
      M5pm1.begin();
      // Enable the PM1 5V boost output (MBUS 5V and 3.3V)
      M5pm1.setExtOutput(true);
      /// KEY1/2/3 are wired to PM1 GPIO0/1/2 (pressed = LOW)
      M5pm1.setGPIOFunction(M5PM1_Class::gpio0, M5PM1_Class::gpio);
      M5pm1.setGPIOFunction(M5PM1_Class::gpio1, M5PM1_Class::gpio);
      M5pm1.setGPIOFunction(M5PM1_Class::gpio2, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio0, M5PM1_Class::input);
      M5pm1.setGPIOMode(M5PM1_Class::gpio1, M5PM1_Class::input);
      M5pm1.setGPIOMode(M5PM1_Class::gpio2, M5PM1_Class::input);
      /// PM1 GPIO4 is the BMI270 INT1 input (motion wakeup)
      M5pm1.setGPIOFunction(M5PM1_Class::gpio4, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio4, M5PM1_Class::input);
      /// PM1 GPIO3 is the IRQ output wired to ESP32 G2. Without an IRQ pin
      /// configured the PM1 auto-clears its IRQ status (0x40-0x42) and the
      /// power button / wake events cannot be detected.
      /// Configure it as a push-pull high output before switching to the IRQ
      /// function, so the released line is actively driven high.
      M5pm1.setGPIOMode(M5PM1_Class::gpio3, M5PM1_Class::output);
      M5pm1.setGPIODrive(M5PM1_Class::gpio3, M5PM1_Class::push_pull);
      M5pm1.setGPIOPull(M5PM1_Class::gpio3, M5PM1_Class::pull_up);
      M5pm1.setGPIOOutput(M5PM1_Class::gpio3, true);
      M5pm1.setGPIOFunction(M5PM1_Class::gpio3, M5PM1_Class::irq);
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
      /// After an EXT1 wakeup the pin is still owned by the RTC IO mux and the
      /// digital GPIO input reads low forever (the release wait on the next
      /// sleep entry would never finish). Return it to the digital function.
      rtc_gpio_deinit((gpio_num_t)_wakeupPin);
#endif
      /// make the PM1 IRQ output readable as the wakeup pin
      m5gfx::pinMode(_wakeupPin, m5gfx::pin_mode_t::input_pullup);
      /// charge detect input (IOE1 G8 = AW32901 CHG_STAT, low = charging)
      M5.getIOExpander(0).setDirection(M5IOE1_Class::gpio8, false);
      /// settle battery presence early (the charger may still be idle).
      _batteryPresent();
      { /// TF card power (IOE1 G1) is off at reset; enable it so the SD card is usable
        auto& ioe1 = M5.getIOExpander(0);
        ioe1.setHighImpedance(M5IOE1_Class::gpio1, false);
        ioe1.setDirection(M5IOE1_Class::gpio1, true);
        ioe1.digitalWrite(M5IOE1_Class::gpio1, true);
      }
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C5)

    /// setup power management ic
    switch (M5.getBoard())
    {
    default:
      break;

    case board_t::board_M5ToughC5:
      _pmic = pmic_t::pmic_m5pm1;
      _wakeupPin = GPIO_NUM_4;
      /// bring up the PM1 early so its status registers are readable below.
      M5pm1.begin();
      /// GPIO4 drives the buzzer through PM1 PWM channel 1. The PM1 keeps
      /// running across ESP resets and retains its PWM state, so put the
      /// channel off at boot, then normalize the pin before selecting its PWM
      /// function. Stopping the channel before sleep is left to the caller:
      /// the PM1 stays powered while the ESP sleeps, so the application may
      /// intend the PWM output to remain active, which makes it application
      /// policy rather than board initialization.
      /// Selecting the PWM function is what makes a retained duty audible
      /// again, so it is only done once the channel is known to be off. If that
      /// cannot be confirmed, the pin is left as a plain output driving low,
      /// which is silent whatever the retained PWM state is.
      bool pwm_off = false;
      for (int retry = 3; !(pwm_off = M5pm1.setPwmDuty12bit(M5PM1_Class::pwm_ch1, 0, pwm_polarity_t::normal, false)) && --retry; )
      {
        m5gfx::delay(10);
      }
      M5pm1.setGPIODrive(M5PM1_Class::gpio4, M5PM1_Class::push_pull);
      M5pm1.setGPIOPull(M5PM1_Class::gpio4, M5PM1_Class::pull_none);
      M5pm1.setGPIOOutput(M5PM1_Class::gpio4, false);
      M5pm1.setGPIOMode(M5PM1_Class::gpio4, M5PM1_Class::output);
      if (pwm_off)
      {
        M5pm1.setGPIOFunction(M5PM1_Class::gpio4, M5PM1_Class::special);
      }
      else
      {
        bool gpio_fallback = false;
        for (int retry = 3; !(gpio_fallback = M5pm1.setGPIOFunction(M5PM1_Class::gpio4, M5PM1_Class::gpio)) && --retry; )
        {
          m5gfx::delay(10);
        }
        if (gpio_fallback)
        {
          M5_LOGE("PM1 PWM ch1 could not be turned off. GPIO4 was switched to GPIO mode.");
        }
        else
        {
          M5_LOGE("PM1 PWM ch1 could not be turned off, and GPIO4 could not be switched to GPIO mode; silence cannot be guaranteed.");
        }
      }
      /// PM1 は常時給電で ESP のリセットを跨いで状態が残るため、直前に動いて
      /// いたファームの設定に依存しないよう IRQ 関連を初期化する
      M5pm1.clearWakeSource();
      M5pm1.clearIRQStatus();
      M5pm1.setGPIOIRQMaskBits(0x16);  // enable GPIO0(TP INT)/GPIO3(RTC nIRQ), disable other GPIO IRQ
      /// PM1 GPIO0 は TP INT 入力。
      M5pm1.setGPIOFunction(M5PM1_Class::gpio0, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio0, M5PM1_Class::input);
      /// settle battery presence early (the charger may still be idle).
      _batteryPresent();
      { /// normalize the charger lines on the IO expander (a previous firmware
        /// may have left them in another state):
        /// CHG_PROG (IOE1 G1) is a resistor-network charge-rate setting that
        /// must stay released, and CHG_STAT (IOE1 G3) is an input.
        auto& ioe1 = M5.getIOExpander(0);
        ioe1.setDirection(M5IOE1_Class::gpio1, false);
        ioe1.setHighImpedance(M5IOE1_Class::gpio1, true);
        if (!ioe1.setPullMode(M5IOE1_Class::gpio1, IOExpander_Base::pull_none))
        {
          M5_LOGE("M5IOE1 CHG_PROG pull state could not be released.");
        }
        ioe1.setDirection(M5IOE1_Class::gpio3, false);
      }
      M5pm1.setBatteryCharge(true);
      M5pm1.setDCDCOutput(true);
      M5pm1.setLDOOutput(true);
      M5pm1.setLedEnLevel(true);
      /// PM1 GPIO1 は ESP32 の G4 へ配線された IRQ 出力。IRQ ピンを設定して
      /// おかないと PM1 が IRQ ステータス (0x40-0x42) を自動クリアしてしまい、
      /// 電源ボタンや RTC アラームの IRQ 検出が機能しない。
      /// IRQ 機能へ切り替える前に push-pull 出力の High として設定しておき、
      /// 解放時に能動的に High が駆動されるようにする。
      M5pm1.setGPIOMode(M5PM1_Class::gpio1, M5PM1_Class::output);
      M5pm1.setGPIODrive(M5PM1_Class::gpio1, M5PM1_Class::push_pull);
      M5pm1.setGPIOPull(M5PM1_Class::gpio1, M5PM1_Class::pull_up);
      M5pm1.setGPIOOutput(M5PM1_Class::gpio1, true);
      M5pm1.setGPIOFunction(M5PM1_Class::gpio1, M5PM1_Class::irq);
      /// PM1 GPIO3 は RX8130 の nIRQ 入力 (外部プルアップ・アクティブ Low)。
      /// 入力にしておくと IRQ ピン設定時のレベル変化スキャン対象になり、
      /// RTC アラームが IRQ 出力 (= ESP32 G4 の Low) として伝わる。
      M5pm1.setGPIOFunction(M5PM1_Class::gpio3, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio3, M5PM1_Class::input);
      /// make the PM1 IRQ output readable as the wakeup pin。
      /// この線には外部プルアップが無く、IRQ 解放時に High へ戻す駆動も
      /// 期待できないため、内部プルアップを有効にする。
      m5gfx::pinMode(_wakeupPin, m5gfx::pin_mode_t::input_pullup);
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32S3)

    /// setup power management ic
    switch (M5.getBoard())
    {
    default:
      break;

    case board_t::board_M5StackCoreS3:
    case board_t::board_M5StackCoreS3SE:
    case board_t::board_M5StackChan:
      _core_s3_aw9523_bit(0x03, 0b10000000, true);  // SY7088 BOOST_EN
      _pmic = Power_Class::pmic_t::pmic_axp2101;
      Axp2101.begin();
      static constexpr std::uint8_t reg_data_array[] =
      { 0x90, 0xBF  // LDOS ON/OFF control 0
      , 0x92, 18 -5 // ALDO1 set to 1.8v // for AW88298
      , 0x93, 33 -5 // ALDO2 set to 3.3v // for ES7210
      , 0x94, 33 -5 // ALDO3 set to 3.3v // for camera
      , 0x95, 33 -5 // ALDO4 set to 3.3v // for TF card slot
      , 0x27, 0x00 // PowerKey Hold=1sec / PowerOff=4sec
      , 0x69, 0x11 // CHGLED setting
      , 0x10, 0x30 // PMU common config
      , 0x30, 0x0F // ADC enabled (for voltage measurement)
      };
      Axp2101.writeRegister8Array(reg_data_array, sizeof(reg_data_array));
      // The touch INT is routed to the ESP32 as TOUCH_INT -> AW9523 P1_2 -> AW9523 INTN
      // -> I2C_INT -> GPIO21. The power key is wired to the AXP2101 PWRON only, and the
      // AXP2101 IRQ pin is shared with the RTC INT, so neither reaches the ESP32.
      // Therefore touch is the only usable wakeup source on this board.
      _wakeupPin = GPIO_NUM_21; // I2C_INT ( AW9523 INTN )
      break;

    case board_t::board_M5StickS3:
      _pmic = pmic_t::pmic_m5pm1;
      M5pm1.setGPIOFunction(M5PM1_Class::gpio0, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio0, M5PM1_Class::input);
      break;

    case board_t::board_M5StopWatch:
      _pmic = pmic_t::pmic_m5pm1;
      {
        M5pm1.setGPIOFunction(M5PM1_Class::gpio2, M5PM1_Class::gpio);
        M5pm1.setGPIOMode(M5PM1_Class::gpio2, M5PM1_Class::input);

        // M5IOE1: PWM1 drives IO9 (G9 motor). REG_PWM_FREQ 0x25/0x26 Hz LE; REG_PWM1_DUTY 0x1B/0x1C (bit7 EN).
        constexpr uint16_t motor_pwm_hz = 2000;
        auto& ioe1 = static_cast<M5IOE1_Class&>(M5.getIOExpander(0));
        if (ioe1.setPwmDuty12bit(M5IOE1_Class::pwm_ch1, 0, pwm_polarity_t::normal, false))
        {
          ioe1.setPwmFrequency(motor_pwm_hz);
          // IO9 (G9 motor / PWM1): push-pull output, duty off until setVibration
          ioe1.setHighImpedance(M5IOE1_Class::gpio9, false);
          ioe1.setDirection(M5IOE1_Class::gpio9, true);
        }
        else
        {
          M5_LOGE("M5IOE1 PWM ch1 could not be turned off. Motor output was not enabled.");
        }
      }
      break;

    case board_t::board_M5StampS3Bat:
      _pmic = pmic_t::pmic_m5pm1;
      /// G3 = CHG_PROG of the charger: driven low = 650mA, left floating = 200mA
      /// (official documentation). Only those two levels are defined, and the
      /// PM1 keeps its state across an ESP reset, so the pin is released to an
      /// input first (before anything else can expose a held high latch),
      /// then the latch is normalized low while it is still an input.
      /// The pull is cleared before the pin becomes an input (a held pull-up
      /// would otherwise show on the pin). Every step is attempted even when
      /// an earlier one failed: each of them only moves the pin towards a
      /// defined state (no pull, input, low latch, push-pull, GPIO mux), so
      /// a transient write failure must not leave a held high latch driven
      /// just because the release before it did not go through.
      (void)M5pm1.setGPIOPull(M5PM1_Class::gpio3, M5PM1_Class::pull_none);
      (void)M5pm1.setGPIOMode(M5PM1_Class::gpio3, M5PM1_Class::input);
      (void)M5pm1.setGPIOOutput(M5PM1_Class::gpio3, false);
      (void)M5pm1.setGPIODrive(M5PM1_Class::gpio3, M5PM1_Class::push_pull);
      (void)M5pm1.setGPIOFunction(M5PM1_Class::gpio3, M5PM1_Class::gpio);
      M5pm1.setGPIOFunction(M5PM1_Class::gpio1, M5PM1_Class::gpio);
      M5pm1.setGPIOFunction(M5PM1_Class::gpio2, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio1, M5PM1_Class::output);
      M5pm1.setGPIOMode(M5PM1_Class::gpio2, M5PM1_Class::input);
      M5pm1.setGPIODrive(M5PM1_Class::gpio1, M5PM1_Class::push_pull);
      break;

    case board_t::board_M5PaperS3:
      m5gfx::pinMode(M5PaperS3_CHG_STAT_PIN, m5gfx::pin_mode_t::input);
      _batAdcCh = ADC1_GPIO3_CHANNEL;
      _batAdcUnit = 1;
      _batAdcPin = 3;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 2.0f;
      _wakeupPin = GPIO_NUM_48; // touch panel INT
      break;

    case board_t::board_M5PaperDIY:
      _pmic = pmic_t::pmic_m5pm1;
      M5pm1.setGPIOFunction(M5PM1_Class::gpio2, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio2, M5PM1_Class::output);
      M5pm1.setGPIODrive(M5PM1_Class::gpio2, M5PM1_Class::push_pull);
      M5pm1.setGPIOOutput(M5PM1_Class::gpio2, true); // EPD_PWR
      M5pm1.setGPIOFunction(M5PM1_Class::gpio3, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio3, M5PM1_Class::input);
      M5pm1.setGPIOPull(M5PM1_Class::gpio3, M5PM1_Class::pull_up); // CHG_STAT, active low
      break;
    
    case board_t::board_M5PaperColor:
      _rtcIntPin = GPIO_NUM_7;
      _pmic = pmic_t::pmic_m5pm1;
      M5pm1.setLDOOutput(true); // RGB PWR EN
      M5pm1.setGPIOFunction(M5PM1_Class::gpio3, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio3, M5PM1_Class::output);
      M5pm1.setGPIODrive(M5PM1_Class::gpio3, M5PM1_Class::push_pull);
      M5pm1.setGPIOOutput(M5PM1_Class::gpio3, true); // TF card power
      break;

    case board_t::board_M5ChainCaptain:
      _pmic = pmic_t::pmic_m5pm1;
      // M5PM1_G0 -- Grove Power
      M5pm1.setGPIOFunction(M5PM1_Class::gpio0, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio0, M5PM1_Class::output);
      M5pm1.setGPIODrive(M5PM1_Class::gpio0, M5PM1_Class::push_pull);
      M5pm1.setGPIOOutput(M5PM1_Class::gpio0, false);
      // M5PM1_G3 -- Chain Power
      M5pm1.setGPIOFunction(M5PM1_Class::gpio3, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio3, M5PM1_Class::output);
      M5pm1.setGPIODrive(M5PM1_Class::gpio3, M5PM1_Class::push_pull);
      M5pm1.setGPIOOutput(M5PM1_Class::gpio3, false);
      {
        auto& ioe1 = M5.getIOExpander(0);
        // M5IOE1_G3 -- Charge Status
        ioe1.setDirection(M5IOE1_Class::gpio3, false);
        ioe1.setPullMode(M5IOE1_Class::gpio3, IOExpander_Base::pull_none);
        // M5IOE1_G4 -- Boost Control
        ioe1.setHighImpedance(M5IOE1_Class::gpio4, false);
        ioe1.setDirection(M5IOE1_Class::gpio4, true);
        ioe1.digitalWrite(M5IOE1_Class::gpio4, false);
      }
      break;
    
    case board_t::board_M5PaperMono:
      _rtcIntPin = GPIO_NUM_1;
      _pmic = pmic_t::pmic_m5pm1;
      _wakeupPin = GPIO_NUM_1; // PY IQR

      M5pm1.clearWakeSource();
      M5pm1.clearIRQStatus();
      M5pm1.setGPIOIRQMaskBits(0x1E);  // enable GPIO0 interrupt, disable other GPIO IRQ

      M5pm1.setGPIOFunction(M5PM1_Class::gpio0, M5PM1_Class::gpio);
      M5pm1.setGPIOMode(M5PM1_Class::gpio0, M5PM1_Class::input);

      M5pm1.setGPIOMode(M5PM1_Class::gpio1, M5PM1_Class::output);
      M5pm1.setGPIODrive(M5PM1_Class::gpio1, M5PM1_Class::push_pull);
      M5pm1.setGPIOPull(M5PM1_Class::gpio1, M5PM1_Class::pull_up);
      M5pm1.setGPIOOutput(M5PM1_Class::gpio1, true);
      M5pm1.setGPIOFunction(M5PM1_Class::gpio1, M5PM1_Class::irq);
      {
        auto& ioe1 = M5.getIOExpander(0);
        ioe1.setHighImpedance(M5IOE1_Class::gpio14, false); // Turn on SD card power
        ioe1.setDirection(M5IOE1_Class::gpio14, true);
        ioe1.digitalWrite(M5IOE1_Class::gpio14, true);
      }

      // Keep IP2316 off the I2C bus until charge control/status is requested.
      init_papermono_ip2315_access();
      break;

    case board_t::board_M5Capsule:
      _batAdcCh = ADC1_GPIO6_CHANNEL;
      _batAdcUnit = 1;
      _batAdcPin = 6;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 2.0f;
      break;

    case board_t::board_M5AirQ:
      _batAdcCh = ADC2_GPIO14_CHANNEL;
      _batAdcUnit = 2;
      _batAdcPin = 14;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 2.0f;
      break;

    case board_t::board_M5DinMeter:
    case board_t::board_M5Cardputer:
    case board_t::board_M5CardputerADV:
      _batAdcCh = ADC1_GPIO10_CHANNEL;
      _batAdcUnit = 1;
      _batAdcPin = 10;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 2.0f;
      break;

    case board_t::board_M5PowerHub:
      M5.In_I2C.writeRegister8(powerhub_i2c_addr, 0x05, 1, i2c_freq); // Enabel VAMeter
      break;
    
    case board_t::board_M5StampPLC:
      _rtcIntPin = GPIO_NUM_14;
      Ina226.begin();
      INA226_Class::config_t cfg;
      cfg.sampling_rate = INA226_Class::Sampling::Rate16;
      cfg.bus_conversion_time = INA226_Class::ConversionTime::US_1100;
      cfg.shunt_conversion_time = INA226_Class::ConversionTime::US_1100;
      cfg.mode = INA226_Class::Mode::ShuntAndBus;
      cfg.shunt_res = 0.01f;
      cfg.max_expected_current = 2.0f;
      Ina226.config(cfg);
      break;
    }

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    /// setup power management ic
    
    switch (M5.getBoard())
    {
    default:
      break;

    case board_t::board_M5TimerCam:
      m5gfx::pinMode(TimerCam_POWER_HOLD_PIN, m5gfx::pin_mode_t::output);
      m5gfx::gpio_hi(TimerCam_POWER_HOLD_PIN);
      m5gfx::pinMode(TimerCam_LED_PIN, m5gfx::pin_mode_t::output);
      m5gfx::gpio_lo(TimerCam_LED_PIN);  // system LED off
      _batAdcCh = ADC1_GPIO38_CHANNEL;
      _batAdcUnit = 1;
      _batAdcPin = 38;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 1.513f;
      break;

    case board_t::board_M5StackCoreInk:
      _wakeupPin = GPIO_NUM_27; // power button;
      _rtcIntPin = GPIO_NUM_19;
      _batAdcCh = ADC1_GPIO35_CHANNEL;
      _batAdcUnit = 1;
      _batAdcPin = 35;
      _pmic = pmic_t::pmic_adc;
      _adc_ratio = 25.1f / 5.1f;
      break;

    case board_t::board_M5Paper:
      m5gfx::pinMode(M5Paper_EXT5V_ENABLE_PIN, m5gfx::pin_mode_t::output);
      _wakeupPin = GPIO_NUM_36; // touch panel INT;
      _batAdcCh = ADC1_GPIO35_CHANNEL;
      _batAdcUnit = 1;
      _batAdcPin = 35;
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
      _wakeupPin = GPIO_NUM_35; // power button;
      m5gfx::pinMode(StickCPlus2_LED_PIN, m5gfx::pin_mode_t::output);
      _batAdcCh = ADC1_GPIO38_CHANNEL;
      _batAdcUnit = 1;
      _batAdcPin = 38;
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

    if (identity_settled)
    {
      _pmic = settled_pmic;
    }
    else if (_pmic == Power_Class::pmic_t::pmic_axp192)
    { /// Both probes read the same ID register (0x03 = AXP192, 0x4A = AXP2101),
      /// so a positive answer from either one settles the identity. A single
      /// transient NACK must not leave the default in place while the other
      /// chip already identified itself, so the pair is retried a few times
      /// until one of them answers.
      for (int retry = 0; retry < 3; ++retry)
      {
        if (Axp192.begin()) { _identity_settled = true; break; }
        if (Axp2101.begin()) { _pmic = Power_Class::pmic_t::pmic_axp2101; _identity_settled = true; break; }
        m5gfx::delay(1);
      }
      /// Without a positive ID the board default stays provisional: the
      /// capability set is published as the default, but the charge state
      /// API reports io_error and the charge setters refuse, so a chip that
      /// was never identified is not read or written with the wrong
      /// register map. A later begin() probes again.
      _identity_unconfirmed = !_identity_settled;
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
        , 0x40, 0x00    // reg40h IRQ 1, all disable
        , 0x41, 0x00    // reg41h IRQ 2, all disable
        , 0x42, 0x03    // reg42h IRQ 3, power key irq enable
        , 0x43, 0x00    // reg43h IRQ 4, all disable
        , 0x44, 0x00    // reg44h IRQ 5, all disable
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
          Axp192.setLDO2(3300);
          static constexpr std::uint8_t reg92h_96h[] =
          { 0x00 // GPIO1 NMOS OpenDrain
          , 0x00 // GPIO2 NMOS OpenDrain
          , 0x00 // GPIO0~2 low
          , 0x05 // GPIO3 NMOS OpenDrain, GPIO4 NMOS OpenDrain
          , 0x00 // GPIO3 low, GPIO4 low
          };
          Axp192.writeRegister(0x92, reg92h_96h, sizeof(reg92h_96h));
          Ina3221[0].begin();
          Ina3221[1].begin();
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
      , 0x30, 0x0F // ADC enabled (for voltage measurement)
      // , 0x18, 0x0E
      };
      Axp2101.writeRegister8Array(reg_data_array, sizeof(reg_data_array));

      // for Core2 v1.1 (AXP2101+INA3221)
      if (Ina3221[0].begin())
      {}
    }


#endif

#if defined (CONFIG_IDF_TARGET_ESP32S3) || defined (CONFIG_IDF_TARGET_ESP32C61) || defined (CONFIG_IDF_TARGET_ESP32C5)
    if (_pmic == pmic_t::pmic_m5pm1)
    {
      M5pm1.begin();
    }
#endif

#endif
    /// The PMIC identity is settled here (on the AXP192 / AXP2101 boards
    /// only once a probe has answered; see the top of this function).
    /// _initialized is raised by M5Unified::begin() once the whole
    /// initialization has completed, not here.
    return (_pmic != pmic_t::pmic_unknown);
  }

#if defined (CONFIG_IDF_TARGET_ESP32S3)

  static constexpr const uint8_t _core_s3_bus_en = 0b00000010; // BUS EN
  static constexpr const uint8_t _core_s3_usb_en = 0b00100000; // USB OTG EN

  // AW9523 の出力ポートを操作する主体 (setExtOutput / setUsbOutput / スピーカー制御) を直列化する。
  // BUS_OUT_EN を落とす手順は 200ms の待ちを挟むので、read-modify-write 同士の交差を排除する必要がある。
  static SemaphoreHandle_t _core_s3_mutex(void)
  {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    static StaticSemaphore_t storage;
    static SemaphoreHandle_t mutex = xSemaphoreCreateMutexStatic(&storage);
#else
    static SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
#endif
    return mutex;
  }
  struct _core_s3_lock_t
  {
    SemaphoreHandle_t mutex;
    bool locked;
    _core_s3_lock_t(void) : mutex { _core_s3_mutex() }, locked { mutex != nullptr && xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE } {}
    ~_core_s3_lock_t(void) { if (locked) { xSemaphoreGive(mutex); } }
  };

  static void _core_s3_output_locked(uint8_t mask, bool enable)
  {
    static constexpr const uint8_t port0_reg = 0x02;
    static constexpr const uint8_t port1_reg = 0x03;
    static constexpr const uint8_t port1_bitmask_boost = 0b10000000; // BOOST_EN

    uint8_t orig[2];
    if (!M5.In_I2C.readRegister(aw9523_i2c_addr, port0_reg, orig, sizeof(orig), i2c_freq)) { return; }

    uint8_t buf[2] = { (uint8_t)(orig[0] | mask), (uint8_t)(orig[1] | port1_bitmask_boost) };

    if (!enable)
    {
      buf[0] = orig[0] & ~mask;
      // if (0 == (p0 & (_core_s3_bus_en | _core_s3_usb_en))) // 両方が無効の場合のみ BOOST_EN を無効化する
      if (0 == (buf[0] & _core_s3_bus_en))
      {
        buf[1] &= ~port1_bitmask_boost;
      }
      if (orig[0] & mask & _core_s3_bus_en)
      {
        // BUS_OUT_EN を 1→0 にするときは BOOST_EN を先に落とし、BUS_OUT が放電してから切り替える。
        // BUS_OUT_EN=0 は BUS 入力側スイッチを ON にする操作なので、BUS_OUT が 5V のまま切り替えると
        // USB VBUS が BUS_OUT へ流れ込み続け、以後 BUS_OUT_EN=0 でも 5V が残る (TS 検出も張り付き、
        // setExtOutput(true) が cancel され続ける)。放電は無負荷で約 80ms、余裕をみて 200ms 待つ。
        // BOOST_EN が読み取り時点で既に 0 でも、いつ 0 になったかは分からないので待ちは省略しない。
        auto restore = [&](void)
        {
          // I2C の失敗後はデバイス側の反映が不確定なので、latch を読み直して実状態から戻す。
          // BUS_OUT_EN が残っていれば BOOST_EN を開始時の値へ、落ちていれば BOOST_EN=0 を確定する。
          // 読めなければ開始時の値を書き戻す (有効化方向なので順序の問題は生じない)。
          // 復旧の書込自体も失敗し得るので、読み戻して一致するまで有界で繰り返す。
          for (int retry = 0; ; ++retry)
          {
            uint8_t cur[2];
            if (M5.In_I2C.readRegister(aw9523_i2c_addr, port0_reg, cur, sizeof(cur), i2c_freq))
            {
              uint8_t want = (cur[0] & _core_s3_bus_en) ? ((cur[1] & ~port1_bitmask_boost) | (orig[1] & port1_bitmask_boost))
                                                        : (cur[1] & ~port1_bitmask_boost);
              if (want == cur[1] || retry >= 3) { return; }
              M5.In_I2C.writeRegister8(aw9523_i2c_addr, port1_reg, want, i2c_freq);
            }
            else
            {
              if (retry >= 3) { return; }
              M5.In_I2C.writeRegister(aw9523_i2c_addr, port0_reg, orig, sizeof(orig), i2c_freq);
            }
            m5gfx::delay(1);
          }
        };
        if (!M5.In_I2C.writeRegister8(aw9523_i2c_addr, port1_reg, orig[1] & ~port1_bitmask_boost, i2c_freq)) { restore(); return; }
        m5gfx::delay(200);
        uint8_t cur[2];
        if (!M5.In_I2C.readRegister(aw9523_i2c_addr, port0_reg, cur, sizeof(cur), i2c_freq)) { restore(); return; }
        if (cur[1] & port1_bitmask_boost) { return; } // BOOST_EN が立て直されている: 放電を保証できないので出力を有効のまま残す
        cur[0] &= ~mask;
        if (!M5.In_I2C.writeRegister(aw9523_i2c_addr, port0_reg, cur, sizeof(cur), i2c_freq)) { restore(); }
        return;
      }
    }
    // 2 バイト一括書きは途中失敗で片方だけ反映され得るので 1 バイトずつ書き、先頭が失敗したら止める。
    // 有効化は BOOST_EN → 出力 EN、無効化は出力 EN → BOOST_EN の順にすると、途中で止まっても
    // 「出力 EN=1 で boost 停止」(出力が死んでいるのに有効と報告される) にはならない。
    if (enable)
    {
      if (!M5.In_I2C.writeRegister8(aw9523_i2c_addr, port1_reg, buf[1], i2c_freq)) { return; }
      M5.In_I2C.writeRegister8(aw9523_i2c_addr, port0_reg, buf[0], i2c_freq);
    }
    else
    {
      if (!M5.In_I2C.writeRegister8(aw9523_i2c_addr, port0_reg, buf[0], i2c_freq)) { return; }
      M5.In_I2C.writeRegister8(aw9523_i2c_addr, port1_reg, buf[1], i2c_freq);
    }
//      Axp2101.setReg0x20Bit0(enable);
  }

  static void _core_s3_output(uint8_t mask, bool enable)
  {
    _core_s3_lock_t lock;
    if (lock.locked) { _core_s3_output_locked(mask, enable); }
  }

  // 無バッテリーで BUS_OUT (TS で検出) に外部 5V が来ている間は、有効化すると自身の給電を断つので取り消す。
  // 既に BUS_OUT_EN=1 なら TS の 5V は自身の出力なので判定しない。
  // 判定に使う読み出しが失敗したときは「危険」側に倒す (読めない状態で有効化しない)。
  static bool _core_s3_ext_output_unsafe(AXP2101_Class& axp)
  {
    uint8_t r00;
    if (!axp.readRegister(0x00, &r00, 1)) { return true; }
    if ((r00 & 0x08) || !(r00 & 0x20)) { return false; } // バッテリーあり、または VBUS なし
    uint8_t p0;
    if (!M5.In_I2C.readRegister(aw9523_i2c_addr, 0x02, &p0, 1, i2c_freq)) { return true; }
    if (p0 & _core_s3_bus_en) { return false; }
    uint8_t ts[2];
    if (!axp.readRegister(0x36, ts, 2)) { return true; }
    std::size_t raw = ((ts[0] & 0x3F) << 8) | ts[1];         // 14bit, 0.5mV/LSB
    if (raw >= (0x3FFF - 32)) { return true; }                // ADC 無効値: 測れていないので危険側 (getTSVoltage の 0V 丸めは表示用)
    return raw > 4000;                                        // 2.0V
  }

  // @return false = cancel した
  static bool _core_s3_set_ext_output(AXP2101_Class& axp, bool enable)
  {
    // 判定と切替を同じ排他区間で行う (判定後に別タスクの off が割り込むと、古い判定で有効化してしまう)。
    // TS の ADC 値は実電圧に最大 0.7s ほど遅れる。自身 (または並行する別タスク) の off 直後は古い 5V を
    // 読み得るので、危険と判定したときは排他を解いて 20ms 待ち、再取得して判定し直す (最大 1s)。
    // 外部給電なら高いままなので cancel になる。待ちの間は他の AW9523 操作を塞がない。
    // 待機中に後発の要求が処理されたら古い要求は破棄する (最後の要求が勝つ)。
    static uint32_t generation = 0;
    uint32_t my_generation = 0;
    uint32_t t0 = 0;
    for (bool first = true; ; first = false)
    {
      {
        _core_s3_lock_t lock;
        if (!lock.locked) { return true; }
        if (first) { my_generation = ++generation; t0 = m5gfx::millis(); } // 待ち時間の起点は排他取得後 (mutex 待ちを含めない)
        else if (my_generation != generation) { return true; }
        if (!enable || !_core_s3_ext_output_unsafe(axp))
        {
          _core_s3_output_locked(_core_s3_bus_en, enable);
          return true;
        }
      }
      if ((m5gfx::millis() - t0) >= 1000) { return false; }
      m5gfx::delay(20);
    }
  }

  void Power_Class::_core_s3_aw9523_bit(uint8_t reg, uint8_t mask, bool on)
  {
    _core_s3_lock_t lock;
    if (!lock.locked) { return; }
    if (on) { M5.In_I2C.bitOn(aw9523_i2c_addr, reg, mask, i2c_freq); }
    else    { M5.In_I2C.bitOff(aw9523_i2c_addr, reg, mask, i2c_freq); }
  }

#endif

  void Power_Class::setExtOutput(bool enable, ext_port_mask_t port_mask)
  {
#if defined (M5UNIFIED_PC_BUILD)
    (void)enable;
    (void)port_mask;
#else
    switch (M5.getBoard())
    {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5CoreP4X:
      {
        auto& ioe1 = M5.getIOExpander(0);
        if (port_mask & ext_port_mask_t::ext_PA)
        {
          ioe1.setHighImpedance(M5IOE1_Class::gpio5, false);
          ioe1.setDirection(M5IOE1_Class::gpio5, true);
          ioe1.digitalWrite(M5IOE1_Class::gpio5, enable);
        }
        if (port_mask & ext_port_mask_t::ext_USB)
        {
          ioe1.setHighImpedance(M5IOE1_Class::gpio2, false);
          ioe1.setDirection(M5IOE1_Class::gpio2, true);
          ioe1.digitalWrite(M5IOE1_Class::gpio2, enable);
        }
      }
      break;

    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
      if (port_mask & ext_port_mask_t::ext_PA)
      {
        auto& ioe = M5.getIOExpander(0);
        ioe.setPullMode(2, enable ? IOExpander_Base::pull_up : IOExpander_Base::pull_down);
        ioe.digitalWrite(2, enable);
      }
      if (M5.getBoard() == board_t::board_M5Tab5X
       && (port_mask & ext_port_mask_t::ext_EXT))
      {
        auto& ioe = M5.getIOExpander(0);
        ioe.setHighImpedance(3, false);
        ioe.setDirection(3, true);
        ioe.digitalWrite(3, enable);
      }
      if (port_mask & ext_port_mask_t::ext_USB)
      {
        auto& ioe = M5.getIOExpander(1);
        ioe.setPullMode(3, enable ? IOExpander_Base::pull_up : IOExpander_Base::pull_down);
        ioe.digitalWrite(3, enable);
      }
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case board_t::board_ArduinoNessoN1:
      M5.getIOExpander(1).digitalWrite(2, enable); // 2 = EXT_PWR_EN
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32C5)
    case board_t::board_M5ToughC5:
      if (_pmic == pmic_t::pmic_m5pm1)
      {
        M5pm1.setExtOutput(enable);
      }
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case board_t::board_M5CoreMatrix:
      { /// IOE1 G5 gates the Grove port power (both the 3.3V rail and the 5V boost)
        auto& ioe1 = M5.getIOExpander(0);
        ioe1.setHighImpedance(M5IOE1_Class::gpio5, false);
        ioe1.setDirection(M5IOE1_Class::gpio5, true);
        ioe1.digitalWrite(M5IOE1_Class::gpio5, enable);
      }
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32H2)

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StackCoreS3:
    case board_t::board_M5StackCoreS3SE:
    case board_t::board_M5StackChan:
      {
        if (!_core_s3_set_ext_output(Axp2101, enable))
        {
          ESP_LOGW("Power","setExtPower(true) is canceled.");
        }
      }
      break;

    case board_t::board_M5StickS3:
    case board_t::board_M5StopWatch:
    case board_t::board_M5PaperColor:
      if (_pmic == pmic_t::pmic_m5pm1)
      {
        M5pm1.setExtOutput(enable);
      }
      break;

    case board_t::board_M5ChainCaptain:
    {
      if (port_mask & ext_port_mask_t::ext_PA)
      {
        M5pm1.setGPIOOutput(M5PM1_Class::gpio0, enable);
      }
      if (port_mask & (ext_port_mask_t::ext_PB1 | ext_port_mask_t::ext_PB2))
      {
        M5pm1.setGPIOOutput(M5PM1_Class::gpio3, enable);
      }
      const bool boost_enabled = M5pm1.getGPIOOutputLatch(M5PM1_Class::gpio0)
                              || M5pm1.getGPIOOutputLatch(M5PM1_Class::gpio3);
      M5.getIOExpander(0).digitalWrite(M5IOE1_Class::gpio4, boost_enabled);
      break;
    }
    case board_t::board_M5StampS3Bat:
      // Use G1 Control 5V output
      M5pm1.setGPIOOutput(M5PM1_Class::gpio1, enable);
      break;

    case board_t::board_M5PowerHub:
      if (port_mask & ext_port_mask_t::ext_USB)
      {
        M5.In_I2C.writeRegister8(powerhub_i2c_addr, 0x01, enable, i2c_freq);
      }
      if (port_mask & ext_port_mask_t::ext_PA)
      {
        M5.In_I2C.writeRegister8(powerhub_i2c_addr, 0x02, enable, i2c_freq);
      }
      if (port_mask & ext_port_mask_t::ext_PC1)
      {
        M5.In_I2C.writeRegister8(powerhub_i2c_addr, 0x03, enable, i2c_freq);
      }
      if (port_mask & ext_port_mask_t::ext_PWR485 || port_mask & ext_port_mask_t::ext_PWRCAN) {
          M5.In_I2C.writeRegister8(powerhub_i2c_addr, 0x04, enable, i2c_freq);
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
          cancel = (enable && (Ina3221[0].getShuntVoltage(0) < 0.0f || Ina3221[0].getShuntVoltage(1) < 0.0f) && (8 >= Axp2101.getBatteryLevel()));
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
#endif
  }

  bool Power_Class::getExtOutput(void)
  {
    switch (M5.getBoard())
    {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5CoreP4X:
      return M5.getIOExpander(0).getWriteValue(M5IOE1_Class::gpio5);

    case board_t::board_M5Tab5:
      return M5.getIOExpander(0).getWriteValue(2);
    case board_t::board_M5Tab5X:
      return M5.getIOExpander(0).getWriteValue(3);

#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case board_t::board_ArduinoNessoN1:
      return M5.getIOExpander(1).getWriteValue(2); // E1-> 2 = EXT_PWR_EN

#elif defined (CONFIG_IDF_TARGET_ESP32H2)

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StackCoreS3:
    case board_t::board_M5StackCoreS3SE:
    case board_t::board_M5StackChan:
      {
        static constexpr const uint32_t port0_bitmask = 0b00000010; // BUS EN
        static constexpr const uint8_t port0_reg = 0x02;
        return M5.In_I2C.readRegister8(aw9523_i2c_addr, port0_reg, i2c_freq) & port0_bitmask;
      }
      break;

    case board_t::board_M5PowerHub:
      uint8_t buf[4];
      if (M5.In_I2C.readRegister(powerhub_i2c_addr, 0x01, buf, sizeof(buf), i2c_freq))
      {
        return (*(uint32_t*)buf != 0);
      }
      return false;
      break;

    case board_t::board_M5StickS3:
    case board_t::board_M5StopWatch:
    case board_t::board_M5PaperColor:
      return M5pm1.getExtOutput();
      break;

    case board_t::board_M5ChainCaptain:
      return M5.getIOExpander(0).getWriteValue(M5IOE1_Class::gpio4)
          && (M5pm1.getGPIOOutputLatch(M5PM1_Class::gpio0)
           || M5pm1.getGPIOOutputLatch(M5PM1_Class::gpio3));
      break;

    case board_t::board_M5StampS3Bat:
      return M5pm1.getGPIOOutputLatch(M5PM1_Class::gpio1);
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
    case board_t::board_M5ToughC5:
      return M5pm1.getExtOutput();
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case board_t::board_M5CoreMatrix:
      return M5.getIOExpander(0).getWriteValue(M5IOE1_Class::gpio5);
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
    (void)enable;
    switch (M5.getBoard())
    {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5CoreP4X:
      M5.getIOExpander(0).digitalWrite(M5IOE1_Class::gpio2, enable);
      break;
#endif

#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StackCoreS3:
    case board_t::board_M5StackCoreS3SE:
    case board_t::board_M5StackChan:
      _core_s3_output(_core_s3_usb_en, enable);
      break;

#endif
    default:
      break;
    }
  }

  bool Power_Class::getUsbOutput(void)
  {
    switch (M5.getBoard())
    {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5CoreP4X:
      return M5.getIOExpander(0).getWriteValue(M5IOE1_Class::gpio2);
#endif

#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StackCoreS3:
    case board_t::board_M5StackCoreS3SE:
    case board_t::board_M5StackChan:
      {
        static constexpr const uint8_t reg = 0x02;
        return M5.In_I2C.readRegister8(aw9523_i2c_addr, reg, i2c_freq) & _core_s3_usb_en;
      }
      break;
#endif

    default:
      break;
    }
    return false;
  }

  void Power_Class::setLed(uint8_t brightness)
  {
#if defined (M5UNIFIED_PC_BUILD)
    (void)brightness;
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    static std::unique_ptr<m5gfx::Light_PWM> led;

    switch (M5.getBoard())
    {
    case board_t::board_M5NanoC6:
      if (led.get() == nullptr)
      {
        led.reset(new m5gfx::Light_PWM());
        auto cfg = led->config();
        cfg.invert = false;
        cfg.pwm_channel = 7;
        cfg.pin_bl = M5NanoC6_LED_PIN;
        led->config(cfg);
        led->init(brightness);
      }
      led->setBrightness(brightness);
      break;
    case board_t::board_ArduinoNessoN1:
      {
        // Cannot set brightness; only off and on
        bool level = (brightness == 0) ? true : false;
        M5.getIOExpander(1).digitalWrite(7, level);  // E1-> 7 = LED
      }
      break;
    default:
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    static std::unique_ptr<m5gfx::Light_PWM> led;

    switch (M5.getBoard())
    {
    case board_t::board_M5PaperS3:
      if (led.get() == nullptr)
      {
        led.reset(new m5gfx::Light_PWM());
        auto cfg = led->config();
        cfg.invert = false;
        cfg.pwm_channel = 7;

        /// M5PaperS3 : LED = GPIO0
        switch (M5.getBoard()) {
        case board_t::board_M5PaperS3:
          cfg.pin_bl = GPIO_NUM_0;
          break;

        default:
          break;
        }
        led->config(cfg);
        led->init(brightness);
      }
      led->setBrightness(brightness);
      break;
    default: break;
    }

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    static std::unique_ptr<m5gfx::Light_PWM> led;

    switch (M5.getBoard())
    {
    case board_t::board_M5StackCore2:
      switch (_pmic)
      {
      case pmic_t::pmic_axp192:
        Axp192.writeRegister8(0x9A, 255-brightness);
        break;

      case pmic_t::pmic_axp2101:
        {
          // Cannot set brightness; only off and on
          uint8_t val = (brightness == 0) ? 0x05 : 0x35;
          Axp2101.writeRegister8(0x69, val);
        }
        break;

      default:
        break;
      }
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
#if defined(M5UNIFIED_PC_BUILD)
    (void)withTimer;
#else
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
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
      case pmic_t::pmic_axp192:
        Axp192.powerOff();
        break;

      case pmic_t::pmic_ip5306:
        Ip5306.setPowerBoostKeepOn(withTimer);
        break;

      case pmic_t::pmic_axp2101:
        Axp2101.powerOff();
        break;

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
      case pmic_t::pmic_axp2101:
        Axp2101.powerOff();
        break;

      case pmic_t::pmic_m5pm1:
        if (!withTimer) {
          M5pm1.powerOff();
        }
        break;

#elif defined (CONFIG_IDF_TARGET_ESP32P4)
      case pmic_t::pmic_m5pm1:
        if (!withTimer) {
          M5pm1.powerOff();
        }
        break;

#elif defined (CONFIG_IDF_TARGET_ESP32C61) || defined (CONFIG_IDF_TARGET_ESP32C5)
      case pmic_t::pmic_m5pm1:
      {
        bool arm_wakeup = withTimer;
        if (!withTimer) {
          int retry = 3;
          while (!M5pm1.powerOff() && --retry) { m5gfx::delay(10); }
          if (!retry)
          { /// 電源を落とせないまま wake source 無しで眠ると、電源ボタンの単押しや
            /// IRQ では復帰できなくなる (PM1 の二重クリックや USB 再接続による
            /// 電源サイクルは可能)。単押しで復帰できるよう IRQ ピンを残して眠る
            M5_LOGE("_powerOff: M5PM1 powerOff failed.");
            arm_wakeup = true;
          }
        }
#if SOC_PM_SUPPORT_EXT1_WAKEUP
        if (arm_wakeup && _wakeupPin < GPIO_NUM_MAX)
        { /// RTC の nIRQ は ESP32 に直結されておらず PM1 の IRQ 出力に集約される
          /// 構成 (ToughC5 等)。IRQ 出力が解放される (High に戻る) のを確認して
          /// から ANY_LOW を武装して眠り、その Low 遷移で deep sleep から復帰
          /// できるようにする。
          int retry = 40;
          while (!_releaseWakeupPin(_wakeupPin) && --retry) { m5gfx::delay(10); }
          if (!retry)
          {
            if (withTimer)
            { /// 解放されない線を ANY_LOW で武装したまま眠ると即時復帰の
              /// 再起動ループになるため、wake 経路を確保できなければ眠らない
              M5_LOGE("_powerOff: cannot release the wakeup pin. not sleeping.");
              M5.Display.wakeup();
              return;
            }
            /// powerOff 失敗時の fallback では武装せず従来通り眠る (ログのみ)
            M5_LOGE("_powerOff: cannot release the wakeup pin.");
          }
          else
          {
            if (ESP_OK != esp_sleep_enable_ext1_wakeup(1ULL << _wakeupPin, ESP_EXT1_WAKEUP_ANY_LOW))
            {
              M5_LOGW("_powerOff: GPIO%d cannot be used as a wakeup source.", (int)_wakeupPin);
            }
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
            if (rtc_gpio_is_valid_gpio((gpio_num_t)_wakeupPin))
            { /// IRQ 線には外部プルアップが無いため、deep sleep 中も有効な
              /// RTC ドメインのプルアップで High を維持する
              rtc_gpio_pullup_en((gpio_num_t)_wakeupPin);
              rtc_gpio_pulldown_dis((gpio_num_t)_wakeupPin);
            }
#endif
          }
        }
#endif
        break;
      }
#endif

      case pmic_t::pmic_unknown:
      default:
#if SOC_PM_SUPPORT_EXT_WAKEUP
        if(_rtcIntPin == GPIO_NUM_MAX && _wakeupPin < GPIO_NUM_MAX)
        {
          esp_sleep_enable_ext0_wakeup((gpio_num_t)_wakeupPin, false);
        }
#endif
        break;
      }
    }

    uint8_t pwrHoldPin = M5.getPin(pin_name_t::power_hold);
    if (pwrHoldPin < GPIO_NUM_MAX)
    {
      // This is a process for models that can be turned off by GPIO control.
      // For PaperS3, the power cannot be turned off simply by setting the GPIO to LOW,
      // so a loop is performed to ensure that the power is turned off by repeatedly outputting a pulse.
      for (int i = 0; i < 5; ++i)
      {
        m5gfx::gpio_lo( pwrHoldPin );
        m5gfx::delay(50);
        m5gfx::gpio_hi( pwrHoldPin );
        m5gfx::delay(50);
      }
    }

    switch (M5.getBoard())
    {
    default: break;
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
      for (int i = 0; i < 10; ++i)
      {
        M5.getIOExpander(1).digitalWrite(4, i & 1); // io1.gpio4 == PWROFF_PLUSE
        m5gfx::delay(50);
      }
      break;

    case board_t::board_M5UnitPoEP4:
      for (int ledPin = 15; ledPin <= 17; ledPin++)
      {
        m5gfx::pinMode(ledPin, m5gfx::pin_mode_t::output);
        m5gfx::gpio_hi(ledPin);
        m5gfx::delay(50);
      }
      break;
#endif

#if defined (CONFIG_IDF_TARGET_ESP32C6)
    case board_t::board_ArduinoNessoN1:
      for (int i = 0; i < 10; ++i)
      {
        M5.getIOExpander(1).digitalWrite(0, i & 1); // io1.pin0 == PWROFF_PULSE
        m5gfx::delay(50);
      }
      break;
#endif

#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5PowerHub:
      uint8_t buf[6]={};
      M5.In_I2C.writeRegister(powerhub_i2c_addr, 0x00, buf, sizeof(buf), i2c_freq);
      M5.In_I2C.writeRegister8(powerhub_i2c_addr, 0xE0, 1, i2c_freq); 
      use_deepsleep = false;
      break;
#endif
    }

    if (use_deepsleep) { esp_deep_sleep_start(); }
    esp_light_sleep_start();
    esp_restart();
#endif
  }

  void Power_Class::_timerSleep(void)
  {
#if !defined (M5UNIFIED_PC_BUILD)

    M5.Display.sleep();
    M5.Display.waitDisplay();

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    switch (M5.getBoard())
    {
    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
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
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32S3)
    switch (M5.getBoard())
    {
    case board_t::board_M5StampPLC:
      M5.getIOExpander(0).resetIrq();
      break;

    default:
      break;
    }
#endif
#endif
    _powerOff(true);
  }

  bool Power_Class::_releaseWakeupPin(std::uint_fast8_t wakeup_pin, bool* clear_comm_ok)
  {
    // clear_comm_ok は「この呼び出し内でクリア通信が一度でも成功したか」を返す。
    // ピンが解放されない理由が「要因がまだ生きている (指が触れている等)」なのか
    // 「デバイスと通信できない (回復見込みなし)」なのかを呼び出し元が区別できる。
    bool comm_ok = false;
    for (int retry = 8; retry > 0; --retry)
    {
      if (m5gfx::gpio_in(wakeup_pin)) { if (clear_comm_ok) { *clear_comm_ok = true; } return true; }
      comm_ok |= M5._clearWakeupInterrupt();
      m5gfx::delay(5);
    }
    if (clear_comm_ok) { *clear_comm_ok = comm_ok; }
    return m5gfx::gpio_in(wakeup_pin);
  }

  void Power_Class::deepSleep(std::uint64_t micro_seconds, bool touch_wakeup)
  {
    if (micro_seconds == 0)
    { // A wakeup time of zero means "do not sleep".
      // ( This check comes before the display is put to sleep. )
      M5_LOGW("deepSleep: micro_seconds is 0. not sleeping. ( use Power.sleep_no_timer to sleep without a timer wakeup )");
      return;
    }
    M5.Display.sleep();
    M5.Display.waitDisplay();
#if defined (M5UNIFIED_PC_BUILD)
    (void)micro_seconds;
    (void)touch_wakeup;
#else
    ESP_LOGD("Power","deepSleep");
#if defined (CONFIG_IDF_TARGET_ESP32C3) || defined (CONFIG_IDF_TARGET_ESP32C6) // || defined (CONFIG_IDF_TARGET_ESP32P4)
    ESP_LOGW("Power","deepSleep: deep sleep is not supported on this target.");
#else

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (_pmic == pmic_t::pmic_ip5306)
    {
      Ip5306.setPowerBoostKeepOn(true);
    }
#endif

    uint_fast8_t wpin = _wakeupPin;
    if (touch_wakeup && (M5.getBoard() == board_t::board_M5PaperMono))
    {
      wpin = GPIO_NUM_4;
    }
    bool pin_wakeup_enabled = false;
    if (touch_wakeup && wpin < GPIO_NUM_MAX)
    {
#if M5UNIFIED_PM_SUPPORT_EXT0
      pin_wakeup_enabled = (ESP_OK == esp_sleep_enable_ext0_wakeup((gpio_num_t)wpin, false));
#elif M5UNIFIED_PM_SUPPORT_EXT1 && SOC_RTCIO_PIN_COUNT > 0
      if (rtc_gpio_is_valid_gpio((gpio_num_t)wpin))
      {
        const uint64_t ext_wakeup_pin_1_mask = 1ULL << wpin;
        // SOC_PM_SUPPORT_EXT1_WAKEUP ( the old name is SOC_PM_SUPPORT_EXT_WAKEUP ) and
        // esp_sleep_enable_ext1_wakeup_io() only exist in recent ESP-IDF. Without these
        // fallbacks the whole branch disappears on older ESP-IDF, and the wakeup pin is
        // then silently ignored on every target that has no EXT0. ( ESP32-S3 / P4 etc. )
 #if defined (ESP_IDF_VERSION_VAL) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
        pin_wakeup_enabled = (ESP_OK == esp_sleep_enable_ext1_wakeup_io(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_LOW));
 #else
        pin_wakeup_enabled = (ESP_OK == esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_LOW));
 #endif
 #if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
        if (pin_wakeup_enabled)
        {
#if defined (CONFIG_IDF_TARGET_ESP32C5) || defined (CONFIG_IDF_TARGET_ESP32C61)
          if (M5.getBoard() == board_t::board_M5ToughC5
           || M5.getBoard() == board_t::board_M5CoreMatrix)
          { // PM1 の IRQ 出力線には外部プルアップが無く、プルダウンすると
            // Low に固定されて wakeup ピンが解放されなくなる。内部プルアップで
            // High を維持し、IRQ アサート (Low) だけを wakeup 条件にする。
            rtc_gpio_pullup_en((gpio_num_t)wpin);
            rtc_gpio_pulldown_dis((gpio_num_t)wpin);
          }
          else
#endif
          { /// TODO: reconsider these pull settings.
            // They came in with this EXT1 branch, which was written for M5Tab5 (ESP32-P4),
            // and every board that reaches here now shares them.
            // Pulling the pin down biases it toward the ANY_LOW wakeup condition, so a pin
            // without an external pull-up would wake up immediately. On CoreS3 this works
            // only because I2C_INT has an external 10k pull-up, and it costs about 60uA
            // through that divider for as long as the device sleeps.
            // Check how the wakeup pin of M5Tab5 is wired before changing this.
            rtc_gpio_pullup_dis((gpio_num_t)wpin);
            rtc_gpio_pulldown_en((gpio_num_t)wpin);
          }
        }
 #endif
      }
#endif
      if (pin_wakeup_enabled)
      {
        bool clear_comm_ok = true;
        int comm_fail = 0;
        while (!_releaseWakeupPin(wpin, &clear_comm_ok))
        {
          if (!clear_comm_ok)
          { // 割り込み要因のクリア通信自体が失敗している。解放されない線を
            // ANY_LOW で待つ構成のまま眠ると即時復帰の再起動ループになるため、
            // 失敗が連続する場合は眠らずに戻る。一時的な失敗 (デバイスの
            // ビジー等) は許容し、成功が挟まれば数え直す。
            // ( 要因が生きているだけなら従来通り解放を待つ )
            if (++comm_fail >= 3)
            {
              M5_LOGE("deepSleep: cannot release the wakeup pin. not sleeping.");
              M5.Display.wakeup();
              return;
            }
          }
          else { comm_fail = 0; }
          // Issue #91, ( M5Paper wakes too soon from deep sleep when touch wakeup is enabled - with solution )
          M5.update();
          m5gfx::delay(10);
        }
      }
      else
      { // The wakeup pin is not an RTC IO. ( ex. M5PaperS3 touch INT = GPIO48 )
        // Such a pin can wake up from light sleep, but not from deep sleep.
        M5_LOGW("deepSleep: GPIO%d cannot be used as a deep sleep wakeup source.", (int)wpin);
      }
    }
    else if (touch_wakeup)
    { // The board has no wakeup pin, so touch_wakeup cannot be honored.
      M5_LOGW("deepSleep: this device has no wakeup pin.");
    }
    if (micro_seconds != sleep_no_timer)
    {
      esp_sleep_enable_timer_wakeup(micro_seconds);
    }
    else
    {
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
      if (!pin_wakeup_enabled)
      { // Neither a timer nor a wakeup pin was configured by this call.
        // Unless another wakeup source has been set up, the device will not wake up until it is reset.
        M5_LOGW("deepSleep: no timer or pin wakeup source is enabled.");
      }
    }
    if (pin_wakeup_enabled && !_releaseWakeupPin(wpin))
    { // Must be done immediately before sleeping. ( see lightSleep )
      M5_LOGW("deepSleep: wakeup pin GPIO%d is still asserted.", (int)wpin);
    }
#endif
    esp_deep_sleep_start();
#endif
  }

  void Power_Class::lightSleep(std::uint64_t micro_seconds, bool touch_wakeup)
  {
    if (micro_seconds == 0)
    { // A wakeup time of zero means "do not sleep".
      M5_LOGW("lightSleep: micro_seconds is 0. not sleeping. ( use Power.sleep_no_timer to sleep without a timer wakeup )");
      return;
    }
#if defined (M5UNIFIED_PC_BUILD)
    (void)micro_seconds;
    (void)touch_wakeup;
#else
    ESP_LOGD("Power","lightSleep");
#if defined (CONFIG_IDF_TARGET_ESP32C3) || defined (CONFIG_IDF_TARGET_ESP32C6) || defined (CONFIG_IDF_TARGET_ESP32H2) || defined (CONFIG_IDF_TARGET_ESP32P4)
    ESP_LOGW("Power","lightSleep: light sleep is not supported on this target.");
#else

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (_pmic == pmic_t::pmic_ip5306)
    {
      Ip5306.setPowerBoostKeepOn(true);
    }
#endif

    uint_fast8_t wpin = _wakeupPin;
    if (touch_wakeup && (M5.getBoard() == board_t::board_M5PaperMono))
    {
      wpin = GPIO_NUM_4;
    }
    bool pin_wakeup_enabled = false;
    bool gpio_wakeup_used = false;
    if (touch_wakeup && wpin < GPIO_NUM_MAX)
    {
#if M5UNIFIED_PM_SUPPORT_EXT0 && SOC_RTCIO_PIN_COUNT > 0
      if (rtc_gpio_is_valid_gpio((gpio_num_t)wpin))
      {
        pin_wakeup_enabled = (ESP_OK == esp_sleep_enable_ext0_wakeup((gpio_num_t)wpin, false));
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);
      }
#endif
      if (!pin_wakeup_enabled)
      { // A pin outside the RTC IO range ( ex. M5PaperS3 touch INT = GPIO48 ), or a target
        // without EXT0, can still wake from light sleep by gpio wakeup.
        gpio_wakeup_used = (ESP_OK == gpio_wakeup_enable((gpio_num_t)wpin, gpio_int_type_t::GPIO_INTR_LOW_LEVEL))
                        && (ESP_OK == esp_sleep_enable_gpio_wakeup());
        pin_wakeup_enabled = gpio_wakeup_used;
      }
      if (pin_wakeup_enabled)
      { // Wait until the wakeup pin is released, otherwise the sleep request is rejected.
        bool clear_comm_ok = true;
        int comm_fail = 0;
        while (!_releaseWakeupPin(wpin, &clear_comm_ok))
        {
          if (!clear_comm_ok)
          { // クリア通信の失敗が連続する場合は解放を待たない。light sleep は
            // 即時復帰しても実行が戻るだけなので deep sleep と違い眠って構わない
            if (++comm_fail >= 3)
            {
              M5_LOGE("lightSleep: cannot release the wakeup pin.");
              break;
            }
          }
          else { comm_fail = 0; }
          m5gfx::delay(10);
        }
      }
      else
      {
        M5_LOGW("lightSleep: wakeup by GPIO%d is not enabled.", (int)wpin);
      }
    }
    else if (touch_wakeup)
    { // The board has no wakeup pin, so touch_wakeup cannot be honored.
      M5_LOGW("lightSleep: this device has no wakeup pin.");
    }
    if (micro_seconds != sleep_no_timer){
      esp_sleep_enable_timer_wakeup(micro_seconds);
    }else{
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
      if (!pin_wakeup_enabled)
      { // Neither a timer nor a wakeup pin was configured by this call.
        M5_LOGW("lightSleep: no timer or pin wakeup source is enabled.");
      }
    }
    if (pin_wakeup_enabled && !_releaseWakeupPin(wpin))
    { // Must be done immediately before sleeping: an event that happens after the wait
      // loop leaves the interrupt asserted, so the sleep request would be rejected or
      // the wakeup source would be dead while sleeping.
      M5_LOGW("lightSleep: wakeup pin GPIO%d is still asserted.", (int)wpin);
    }
    esp_light_sleep_start();
    if (gpio_wakeup_used)
    {
      gpio_wakeup_disable((gpio_num_t)wpin);
    }
#endif // CONFIG_IDF_TARGET_ESP32C3 / C6 / C5
#endif // M5UNIFIED_PC_BUILD
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
    M5.Rtc.clearIRQ();
    M5.Rtc.setAlarmIRQ(seconds);
#if !defined (M5UNIFIED_PC_BUILD)
    esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
#endif
    _timerSleep();
  }

  void Power_Class::timerSleep( const rtc_time_t& time)
  {
    M5.Rtc.disableIRQ();
    M5.Rtc.clearIRQ();
    M5.Rtc.setAlarmIRQ(time);
    _timerSleep();
  }

  void Power_Class::timerSleep( const rtc_date_t& date, const rtc_time_t& time)
  {
    M5.Rtc.disableIRQ();
    M5.Rtc.clearIRQ();
    M5.Rtc.setAlarmIRQ(date, time);
    _timerSleep();
  }

  std::int32_t Power_Class::_getBatteryAdcRaw(void)
  {
#if defined (M5UNIFIED_PC_BUILD)
    return 0;
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3)

#if defined (ESP_IDF_VERSION_VAL)
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0) || (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 7) && ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0))
  #define ADC_RAW_ATTEN ADC_ATTEN_DB_12
 #endif
#endif
#ifndef ADC_RAW_ATTEN
#define ADC_RAW_ATTEN ADC_ATTEN_DB_11
#endif

#if defined (M5UNIFIED_BATADC_USE_ARDUINO)

    // The default attenuation (11dB) and calibration are owned consistently
    // by the Arduino core; do not fight it with per-pin overrides here.
    return analogReadMilliVolts(_batAdcPin);

#elif __has_include (<esp_adc/adc_oneshot.h>)

    static adc_oneshot_unit_handle_t adc_handle;
    if (adc_handle == nullptr) {
      adc_oneshot_unit_init_cfg_t init_config;
      memset(&init_config, 0, sizeof(init_config));
      init_config.unit_id = _batAdcUnit == 1 ? ADC_UNIT_1 : ADC_UNIT_2;
      if (adc_oneshot_new_unit(&init_config, &adc_handle) != ESP_OK || adc_handle == nullptr) {
        adc_handle = nullptr;
        return 0;
      }

      adc_oneshot_chan_cfg_t config;
      config.atten = ADC_RAW_ATTEN;
      config.bitwidth = ADC_BITWIDTH_12;
      if (adc_oneshot_config_channel(adc_handle, (adc_channel_t)_batAdcCh, &config) != ESP_OK) {
        adc_oneshot_del_unit(adc_handle);
        adc_handle = nullptr;
        return 0;
      }
    }
    static adc_cali_handle_t adc_cali;
    if (adc_cali == nullptr) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_curve_fitting_config_t cali_config;
        cali_config.unit_id = _batAdcUnit == 1 ? ADC_UNIT_1 : ADC_UNIT_2;
        cali_config.chan = (adc_channel_t)_batAdcCh;
        cali_config.atten = ADC_ATTEN_DB_12;
        cali_config.bitwidth = ADC_BITWIDTH_12;
        adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_line_fitting_config_t cali_config;
        cali_config.unit_id = _batAdcUnit == 1 ? ADC_UNIT_1 : ADC_UNIT_2;
        cali_config.atten = ADC_ATTEN_DB_12;
        cali_config.bitwidth = ADC_BITWIDTH_12;
        adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali);
#endif
    }
    int raw, volt;
    if (adc_oneshot_read(adc_handle, (adc_channel_t)_batAdcCh, &raw) != ESP_OK) {
      return 0;
    }
    // Callers treat the return value as millivolts; without a calibration
    // scheme the raw count cannot be expressed in mV, so report 0 (unreadable)
    // instead of a bogus value.
    if (adc_cali == nullptr) {
      return 0;
    }
    if (adc_cali_raw_to_voltage(adc_cali, raw, &volt) != ESP_OK) {
      return 0;
    }
    return volt;

#else
    static constexpr int BASE_VOLATAGE = 3600;

    static esp_adc_cal_characteristics_t* adc_chars = nullptr;
    if (adc_chars == nullptr)
    {
      if (_batAdcUnit == 2) {
        adc2_config_channel_atten((adc2_channel_t)_batAdcCh, ADC_RAW_ATTEN);
      } else {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten((adc1_channel_t)_batAdcCh, ADC_RAW_ATTEN);
      }
      adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
      esp_adc_cal_characterize((adc_unit_t)_batAdcUnit, ADC_RAW_ATTEN, ADC_WIDTH_BIT_12, BASE_VOLATAGE, adc_chars);
    }
    int raw;
    if (_batAdcUnit == 2) {
      adc2_get_raw((adc2_channel_t)_batAdcCh, adc_bits_width_t::ADC_WIDTH_BIT_12, &raw);
    } else {
      raw = adc1_get_raw((adc1_channel_t)_batAdcCh);
    }
    return esp_adc_cal_raw_to_voltage(raw, adc_chars);
#endif

#else
    return 0;
#endif
  }

  int16_t Power_Class::getVBUSVoltage(void)
  {
    float f = NAN;
#if !defined (M5UNIFIED_PC_BUILD)
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case pmic_t::pmic_m5pm1:
      return M5pm1.getVBUSVoltage();
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case pmic_t::pmic_m5pm1:
      return M5pm1.getVBUSVoltage();
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    case pmic_t::pmic_axp192:
      f = Axp192.getVBUSVoltage();
      break;

#endif

    case pmic_t::pmic_axp2101:
      f = Axp2101.getVBUSVoltage();
      break;

#if defined (CONFIG_IDF_TARGET_ESP32S3) || defined (CONFIG_IDF_TARGET_ESP32C5)
    case pmic_t::pmic_m5pm1:
      f = M5pm1.getVBUSVoltage() / 1000.0f;
      break;
#endif

#endif

    default:
      break;
    }

#endif
    if (isfinite(f))
    {
      return f * 1000;
    }
    return -1;
  }

#if defined (CONFIG_IDF_TARGET_ESP32C5) || defined (CONFIG_IDF_TARGET_ESP32C61)
  /// read the raw charger CHG_STAT line (low = charging).
  bool Power_Class::_readChargeStat(bool* level)
  {
    switch (M5.getBoard())
    {
#if defined (CONFIG_IDF_TARGET_ESP32C5)
    case board_t::board_M5ToughC5:
      return M5.getIOExpander(0).getInputLevel(M5IOE1_Class::gpio3, level);
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case board_t::board_M5CoreMatrix:
      return M5.getIOExpander(0).getInputLevel(M5IOE1_Class::gpio8, level);
#endif
    default:
      return false;
    }
  }

  /// A batteryless charger can hold CHG_STAT low against a collapsed node,
  /// so a low CHG_STAT alone does not prove charge current (ToughC5 only;
  /// the CoreMatrix retry blips are filtered by the 100ms streak below).
  bool Power_Class::_vbatNodeDown(bool* io_ok)
  {
#if defined (CONFIG_IDF_TARGET_ESP32C5)
    bool powered;
    bool ok = M5pm1.getVbatNodePowered(&powered);
    if (io_ok) { *io_ok = ok; }
    return ok && !powered;
#else
    if (io_ok) { *io_ok = true; }
    return false;
#endif
  }

  /// CoreMatrix / ToughC5: with no battery attached, the PM1 VBAT ADC follows
  /// whatever the charger does to the empty node, so presence cannot be read
  /// directly. It is resolved without blocking:
  /// - charging disabled: a collapsed node decides "none" at once; a high
  ///   reading counts as a battery once the node has settled after charging
  ///   stopped (a reset also stops charging, so begin() reuses this path).
  /// - charging enabled: a sustained low CHG_STAT with the VBAT node held up
  ///   counts as present; a VBAT peak above any real battery, or a collapse
  ///   held across samples, counts as absent; steady samples point to a
  ///   battery. Flipping an existing verdict needs a longer streak than the
  ///   initial one, and instability alone never revokes "present".
  /// Until the first verdict, -1 (unknown) is reported and the battery APIs
  /// pass that on instead of guessing. (On the CoreMatrix a detach while
  /// charging can go unnoticed until charging is disabled.)
  /// Every piece of presence evidence (the CHG_STAT low streak, the VBAT
  /// sample baseline and its stable / unstable / low counters) only means
  /// something as an unbroken sequence of successful, charger-enabled
  /// observations. A failed read, a disabled charger, or a charge enable
  /// switch is a gap: the evidence is dropped so nothing observed on the far
  /// side of the gap can complete a streak or a sample count that began
  /// before it. The verdict itself is kept.
  void Power_Class::_bp_dropEvidence(void)
  {
    _bp_chg_low_ms = 0;
    _bp_last_ms = 0;
    _bp_stable = 0;
    _bp_unstable = 0;
    _bp_low = 0;
  }

  std::int8_t Power_Class::_batteryPresent(bool* io_ok)
  {
    /// io_ok reports whether every read of this evaluation succeeded. The
    /// verdict itself is cached across failed reads (a transient NACK must not
    /// flip the presence), so a caller that has to distinguish "read failed"
    /// from "last known verdict" looks at io_ok, not at the return value.
    /// A failed read ends the evaluation with the cached verdict and drops
    /// the transient evidence (see _bp_dropEvidence).
    if (io_ok) { *io_ok = true; }
    bool chg_enabled = true;
    if (!M5pm1.getBatteryCharge(&chg_enabled)) { if (io_ok) { *io_ok = false; } _bp_dropEvidence(); return _batt_present; }
    if (!chg_enabled)
    { /// a disabled charger breaks the continuity of the enabled-path
      /// evidence, and nothing observed here is kept as a baseline for it:
      /// the first sample after re-enabling starts the sampling afresh. The
      /// settle rule below has its own timer (_chg_off_ms).
      _bp_dropEvidence();
      /// with the charger idle there is no float voltage: a collapsed node
      /// proves "no battery" at once. A high reading is trusted as "present"
      /// only once the node has settled after charging stopped (the initial
      /// _chg_off_ms = 0 gives a boot the same settle window).
      std::uint16_t mv = 0;
      if (!M5pm1.getBatteryVoltage(&mv)) { if (io_ok) { *io_ok = false; } }
      else if (mv <= 2600) { _batt_present = 0; }
      else if ((m5gfx::millis() - _chg_off_ms) > 1500) { _batt_present = 1; }
      return _batt_present;
    }

    std::uint32_t now = m5gfx::millis();

    bool chg_stat;
    if (!_readChargeStat(&chg_stat)) { if (io_ok) { *io_ok = false; } _bp_dropEvidence(); return _batt_present; }
    {
      bool node_ok;
      bool node_down = _vbatNodeDown(&node_ok);
      if (!node_ok) { if (io_ok) { *io_ok = false; } _bp_dropEvidence(); return _batt_present; }
      if (!chg_stat && !node_down)
      { /// low = charging into a live node; require a >=100ms streak so a
        /// batteryless retry blip cannot pass as real charge current.
        if (_bp_chg_low_ms == 0) { _bp_chg_low_ms = now ? now : 1; }
        else if ((now - _bp_chg_low_ms) >= 100)
        {
          _batt_present = 1;
        }
        /// real charge current: skip the VBAT rules (a deeply discharged
        /// battery can sit below the collapse threshold while charging).
        return _batt_present;
      }
      /// otherwise fall through and let the VBAT rules decide.
      _bp_chg_low_ms = 0;
    }

    std::uint16_t mv = 0;
    if (!M5pm1.getBatteryVoltage(&mv)) { if (io_ok) { *io_ok = false; } _bp_dropEvidence(); return _batt_present; }

    if (mv > 4450)
    { /// only the batteryless sawtooth peaks above any real battery
      _batt_present = 0;
      _bp_stable = 0;
      _bp_unstable = 0;
      _bp_low = 0;
      return _batt_present;
    }

    if (_bp_last_ms == 0)
    {
      _bp_last_ms = now ? now : 1;
      _bp_last_mv = mv;
      return _batt_present;
    }
    if ((now - _bp_last_ms) < 1250) { return _batt_present; }

    /// the register has refreshed since the previous sample
    std::int32_t diff = (std::int32_t)mv - (std::int32_t)_bp_last_mv;
    if (diff < 0) { diff = -diff; }
    _bp_last_ms = now ? now : 1;
    _bp_last_mv = mv;

    if (mv < 2600) { ++_bp_low; } else { _bp_low = 0; }
    if (diff > 300) { ++_bp_unstable; _bp_stable = 0; }
    else            { ++_bp_stable; _bp_unstable = 0; }

    if (_batt_present < 0)
    {
      if (_bp_low >= 2 || _bp_unstable >= 2) { _batt_present = 0; }
      else if (_bp_stable >= 2 && mv >= 2600) { _batt_present = 1; }
    }
    else if (_batt_present == 0)
    { /// a battery attached later holds the node steady in the plausible
      /// range; the sawtooth cannot hold still this long. (also catches an
      /// attached full battery, which never asserts CHG_STAT.)
      if (_bp_stable >= 4 && mv >= 2600) { _batt_present = 1; }
    }
    else
    { /// present is revoked only by a collapsed node held across samples
      if (_bp_low >= 2) { _batt_present = 0; }
    }
    return _batt_present;
  }

  /// ToughC5 / CoreMatrix: CHG_STAT behind the battery presence gate.
  /// With no battery the charger retries periodically and CHG_STAT blips low,
  /// so the presence has to be decided before the line is believed.
  charge_state_t Power_Class::_chargeStateFromChgStat(void)
  {
    /// Procedure order: the presence gate first (its own reads decide io_error
    /// and undetermined, and "no battery" settles not_charging without the
    /// line), then CHG_STAT only when a battery is there.
    bool io_ok;
    std::int8_t present = _batteryPresent(&io_ok);
    if (!io_ok) { return charge_state_t::io_error; }
    if (present < 0) { return charge_state_t::undetermined; }
    if (present == 0) { return charge_state_t::not_charging; }
    bool level;
    if (!_readChargeStat(&level)) { return charge_state_t::io_error; }
    return level ? charge_state_t::not_charging : charge_state_t::charging;
  }
#endif

  int16_t Power_Class::getBatteryVoltage(void)
  {
#if !defined (M5UNIFIED_PC_BUILD)
    switch (_pmic)
    {

#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case pmic_t::pmic_aw32001:
      return Bq27220.getVoltage_mV();
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case pmic_t::pmic_m5pm1:
      { /// 0 = no battery / -1 = not yet determined (see _batteryPresent)
        std::int8_t bp = _batteryPresent();
        if (bp <= 0) { return bp; }
      }
      return M5pm1.getBatteryVoltage();
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case pmic_t::pmic_m5pm1:
      return M5pm1.getBatteryVoltage();
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_ip5306:
      break;

    case pmic_t::pmic_axp192:
      return Axp192.getBatteryVoltage() * 1000;

#endif

    case pmic_t::pmic_axp2101:
      return Axp2101.getBatteryVoltage() * 1000;

#if defined (CONFIG_IDF_TARGET_ESP32S3) || defined (CONFIG_IDF_TARGET_ESP32C5)
    case pmic_t::pmic_m5pm1:
#if defined (CONFIG_IDF_TARGET_ESP32C5)
      { /// 0 = no battery / -1 = not yet determined (see _batteryPresent)
        std::int8_t bp = _batteryPresent();
        if (bp <= 0) { return bp; }
      }
#endif
      return M5pm1.getBatteryVoltage();
#endif

#endif

    case pmic_t::pmic_adc:
      return _getBatteryAdcRaw() * _adc_ratio;

    default:
      switch (M5.getBoard()) {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
      case board_t::board_M5Tab5:
      case board_t::board_M5Tab5X:
        return Ina226.getBusVoltage() * 1000;
#endif

#if defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5PowerHub:
        uint8_t buf[2];
        if (M5.In_I2C.readRegister(powerhub_i2c_addr, 0x30, buf, sizeof(buf), i2c_freq)) return (buf[1] << 8) | buf[0];
        return 0;
#endif
      default:
        return 0;
      }
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
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case pmic_t::pmic_aw32001:
      mv = Bq27220.getVoltage_F() * 1000;
      if (isnan(mv)) {
        return -1; // Error
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case pmic_t::pmic_m5pm1:
      {
        // Get battery voltage in mV
        int16_t bat_mv = getBatteryVoltage();
        if (bat_mv <= 0) {
          return -1; // Error reading voltage
        }
        mv = bat_mv;
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case pmic_t::pmic_m5pm1:
      {
        int16_t bat_mv = getBatteryVoltage();
        if (bat_mv <= 0) {
          return -1;
        }
        mv = bat_mv;
      }
      break;
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

#if defined (CONFIG_IDF_TARGET_ESP32S3) || defined (CONFIG_IDF_TARGET_ESP32C5)
    case pmic_t::pmic_m5pm1:
      {
        // Get battery voltage in mV
        int16_t bat_mv = getBatteryVoltage();
        if (bat_mv <= 0) {
          return -1; // Error reading voltage
        }
        mv = bat_mv;
      }
      break;
#endif

#endif

    case pmic_t::pmic_adc:
      mv = _getBatteryAdcRaw() * _adc_ratio;
      break;

    default:
      switch (M5.getBoard()) {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
      case board_t::board_M5Tab5:
      case board_t::board_M5Tab5X:
        // 2S Li-Po ( * 1000 / 2 == * 500)
        mv = Ina226.getBusVoltage() * 500;
        break;
#endif

#if defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5PowerHub:
        mv = getBatteryVoltage() / 2;
        break;
#endif
      default:
        return -2;
      }
    }

    int level = (mv - 3300) * 100 / (float)(4150 - 3350);

    return (level < 0) ? 0
         : (level >= 100) ? 100
         : level;
#endif
  }

  bool Power_Class::setBatteryCharge(bool enable)
  {
    (void)enable;   // some chip builds have no control path at all
    if (_identity_unconfirmed) { return false; }   // see begin(): the PMIC never answered its ID probe
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case pmic_t::pmic_aw32001:
      return Aw32001.setBatteryCharge(enable);
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case pmic_t::pmic_m5pm1:
      /// the presence check must not read VBAT before the node collapses,
      /// and a charge enable switch is a gap in its evidence.
      if (!enable) { _chg_off_ms = m5gfx::millis(); }
      _bp_dropEvidence();
      return M5pm1.setBatteryCharge(enable);
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case pmic_t::pmic_m5pm1:
      return M5pm1.setBatteryCharge(enable);
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_ip5306:
      return Ip5306.setBatteryCharge(enable);

    case pmic_t::pmic_axp192:
      return Axp192.setBatteryCharge(enable);

#endif

    case pmic_t::pmic_axp2101:
      return Axp2101.setBatteryCharge(enable);

#if defined (CONFIG_IDF_TARGET_ESP32S3) || defined (CONFIG_IDF_TARGET_ESP32C5)
    case pmic_t::pmic_m5pm1:
      {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
        // M5PaperColor does not support charge control
        // (the PM1 CHG_EN_PP pin is not wired to the charger)
        if (M5.getBoard() == board_t::board_M5PaperColor) {
          return false;
        }
        // M5PaperMono: charging is controlled by the IP2316 charger, not PM1.
        if (M5.getBoard() == board_t::board_M5PaperMono) {
          /// every write of the sequence counts: a gate left in the wrong
          /// state after a failed close is not a success.
          bool res = set_papermono_ip2315_enabled(true);
          if (res && wait_papermono_ip2315_ready()) {
            res = enable ? M5.In_I2C.bitOn (ip2315_i2c_addr, 0x01, 1 << 0, i2c_freq)
                         : M5.In_I2C.bitOff(ip2315_i2c_addr, 0x01, 1 << 0, i2c_freq);
          } else {
            res = false;
          }
          res = set_papermono_ip2315_enabled(false) && res;
          return res;
        }
#endif
#if defined (CONFIG_IDF_TARGET_ESP32C5)
        /// the presence check must not read VBAT before the node collapses,
        /// and a charge enable switch is a gap in its evidence.
        if (!enable) { _chg_off_ms = m5gfx::millis(); }
        _bp_dropEvidence();
#endif
        return M5pm1.setBatteryCharge(enable);
      }
#endif

#endif

    default:
      switch (M5.getBoard()) {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
      case board_t::board_M5Tab5:
      case board_t::board_M5Tab5X:
        /// CHG_EN (IOE1 G7) is owned by this function alone; setChargeCurrent
        /// only selects the QC step.
        return M5.getIOExpander(1).digitalWrite(7, enable);
#endif

#if defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5PowerHub:
        return M5.In_I2C.writeRegister8(powerhub_i2c_addr, 0x06, enable, i2c_freq);
#endif
      default:
        break;
      }
      break;
    }
    return false;
  }

  bool Power_Class::setChargeCurrent(std::uint16_t max_mA, std::uint16_t* applied_mA)
  { (void)max_mA; (void)applied_mA;   // some chip builds have no control path at all
    if (_identity_unconfirmed) { return false; }   // see begin(): the PMIC never answered its ID probe
    if (max_mA == 0)
    { /// 0 is not a step (where a current path exists it selects the lowest
      /// one). Warn once: code written for the old Tab5 / Tab5X behaviour
      /// used 0 to stop charging. The flag only limits the log output.
      static bool warned = false;
      if (!warned)
      {
        warned = true;
        M5_LOGW("setChargeCurrent(0): 0 is not a charge current step. Use setBatteryCharge(false) to stop charging.");
      }
    }
    /// The step contract: the highest step not above max_mA, clamped up to the
    /// lowest step when the request is under all of them. 0 is not a step
    /// (setBatteryCharge(false) stops charging), and applied_mA is only written
    /// when the write actually went through.
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case pmic_t::pmic_aw32001:
      return Aw32001.setChargeCurrent(max_mA, applied_mA);
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case pmic_t::pmic_m5pm1:
      if (M5.getBoard() == board_t::board_M5CoreMatrix)
      { /// IOE1 G3 selects between two steps: driven low = 650mA, released to
        /// an input = 180mA.
        auto& ioe1 = M5.getIOExpander(0);
        const bool select_650mA = (max_mA >= 650);
        bool res = ioe1.setPullMode(M5IOE1_Class::gpio3, IOExpander_Base::pull_none);
        if (select_650mA)
        {
          res = ioe1.digitalWrite(M5IOE1_Class::gpio3, false) && res;
          res = ioe1.setHighImpedance(M5IOE1_Class::gpio3, false) && res;
          res = ioe1.setDirection(M5IOE1_Class::gpio3, true) && res;
        }
        else
        {
          res = ioe1.setDirection(M5IOE1_Class::gpio3, false) && res;
        }
        if (!res) { return false; }
        if (applied_mA) { *applied_mA = select_650mA ? 650 : 180; }
        return true;
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case pmic_t::pmic_m5pm1:
      /// CoreP4X has no charge current control path.
      (void)max_mA;
      break;
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_ip5306:
      return Ip5306.setChargeCurrent(max_mA, applied_mA);

    case pmic_t::pmic_axp192:
      return Axp192.setChargeCurrent(max_mA, applied_mA);

#endif

    case pmic_t::pmic_axp2101:
      return Axp2101.setChargeCurrent(max_mA, applied_mA);

#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case pmic_t::pmic_m5pm1:
      if (M5.getBoard() == board_t::board_M5StampS3Bat)
      { /// PM1 G3 = CHG_PROG: driven low selects 650mA, left floating (input,
        /// no pull) selects 200mA. The pin is never driven high.
        const bool select_650mA = (max_mA >= 650);
        /// The whole pin setup is part of the transaction, including the mux
        /// (mode and latch only take effect while the pin is muxed to GPIO).
        /// The pin is released to an input with no pull first, the low latch
        /// and driver type are set, the mux is switched while the pin is
        /// still an input, and only then (650mA) does it become an output.
        /// Every releasing step is attempted even after an earlier failure
        /// (each one only moves the pin towards a defined state, so a held
        /// high latch is not left driven by a failed release), while the
        /// output switch runs only when every step before it went through.
        /// applied_mA is only reported when the whole transaction succeeded.
        bool res = true;
        res = M5pm1.setGPIOPull(M5PM1_Class::gpio3, M5PM1_Class::pull_none) && res;
        res = M5pm1.setGPIOMode(M5PM1_Class::gpio3, M5PM1_Class::input) && res;
        res = M5pm1.setGPIOOutput(M5PM1_Class::gpio3, false) && res;
        res = M5pm1.setGPIODrive(M5PM1_Class::gpio3, M5PM1_Class::push_pull) && res;
        res = M5pm1.setGPIOFunction(M5PM1_Class::gpio3, M5PM1_Class::gpio) && res;
        if (select_650mA)
        {
          res = res && M5pm1.setGPIOMode(M5PM1_Class::gpio3, M5PM1_Class::output);
        }
        if (!res) { return false; }
        if (applied_mA) { *applied_mA = select_650mA ? 650 : 200; }
        return true;
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
    case pmic_t::pmic_m5pm1:
      if (M5.getBoard() == board_t::board_M5ToughC5)
      {
        // ToughC5 CHG_PROG is IOE1 G1: low selects 830 mA, high selects 180 mA.
        // Set the latch before enabling push-pull output to avoid a transient
        // selection of the opposite current during the mode transition.
        auto& ioe1 = M5.getIOExpander(0);
        const bool select_180mA = max_mA < 830;
        bool res = ioe1.setPullMode(M5IOE1_Class::gpio1, IOExpander_Base::pull_none);
        res = ioe1.digitalWrite(M5IOE1_Class::gpio1, select_180mA) && res;
        res = ioe1.setHighImpedance(M5IOE1_Class::gpio1, false) && res;
        res = ioe1.setDirection(M5IOE1_Class::gpio1, true) && res;
        if (!res) { return false; }
        if (applied_mA) { *applied_mA = select_180mA ? 180 : 830; }
        return true;
      }
      break;
#endif

#endif

    default:
      break;
    }

    switch (M5.getBoard()) {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
      { /// Two steps, selected by QC (IOE1 G5, active low): 500mA / 1000mA.
        /// CHG_EN (G7) is not touched here - it belongs to setBatteryCharge().
        const bool select_1000mA = (max_mA >= 1000);
        if (!M5.getIOExpander(1).digitalWrite(5, !select_1000mA)) { return false; }
        if (applied_mA) { *applied_mA = select_1000mA ? 1000 : 500; }
        return true;
      }
#endif
    default:
      break;
    }
    return false;
  }

  bool Power_Class::_readBatteryCurrent(std::int32_t* mA)
  { /// The boards whose charge state is decided by the battery current read it
    /// through here, so that a failed read stays distinguishable from 0mA.
    /// The public getBatteryCurrent() keeps folding a failure into 0.
    if (mA == nullptr) { return false; }
    switch (M5.getBoard())
    {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
      { // The shunt is wired so that charge current reads negative; invert to
        // match the documented convention (+ = charge / - = discharge).
        float ampere;
        if (!Ina226.readShuntCurrent(&ampere)) { return false; }
        *mA = (std::int32_t)(-1000.0f * ampere);
        return true;
      }
#endif

#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5PowerHub:
      {
        uint8_t buf[2];
        if (!M5.In_I2C.readRegister(powerhub_i2c_addr, 0x32, buf, sizeof(buf), i2c_freq)) { return false; }
        *mA = -(std::int16_t)((buf[1] << 8) | buf[0]);
        return true;
      }
#endif
    default:
      break;
    }
    return false;
  }

  int32_t Power_Class::getBatteryCurrent(void)
  {
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
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
      return 1000.0f * Ina3221[0].getCurrent(0); // 0=CH1. CH1=BAT Current.

#endif

#endif

    default:
      switch (M5.getBoard()) {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
      case board_t::board_M5Tab5:
      case board_t::board_M5Tab5X:
#endif
#if defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5PowerHub:
#endif
#if defined (CONFIG_IDF_TARGET_ESP32P4) || defined (CONFIG_IDF_TARGET_ESP32S3)
        { /// a failed read is reported as 0mA here; getChargeState() uses the
          /// checked path instead.
          std::int32_t mA = 0;
          _readBatteryCurrent(&mA);
          return mA;
        }
#endif
      default:
        return 0;
      }
    }
  }

  bool Power_Class::setChargeVoltage(std::uint16_t max_mV, std::uint16_t* applied_mV)
  {
    (void)max_mV; (void)applied_mV;   // some chip builds have no control path at all
    if (_identity_unconfirmed) { return false; }   // see begin(): the PMIC never answered its ID probe
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    /// AW32001_Class::setChargeVoltage exists but is deliberately not wired up:
    /// its step selection is defective below the lowest step and corrupts the
    /// neighbouring fields. It is fixed separately, and until then the voltage
    /// capability bit stays clear.
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    case pmic_t::pmic_ip5306:
      return Ip5306.setChargeVoltage(max_mV, applied_mV);

    case pmic_t::pmic_axp192:
      return Axp192.setChargeVoltage(max_mV, applied_mV);

#endif

    case pmic_t::pmic_axp2101:
      return Axp2101.setChargeVoltage(max_mV, applied_mV);

#endif

    default:
      break;
    }
    return false;
  }

  /// The one place where the reportable state set of each model lives.
  /// getChargeState() and getChargeStateCaps() both read it, so the advertised
  /// set and the answered value cannot drift apart.
  /// The identity of a model is the pair (pmic, board): several board ids ship
  /// with different PMICs, and the PMIC is only settled during begin().
  static charge_state_set_t _charge_state_caps(Power_Class::pmic_t pmic, board_t board)
  {
    switch (pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case Power_Class::pmic_t::pmic_aw32001:
      return charge_state_t::charging | charge_state_t::not_charging
           | charge_state_t::full     | charge_state_t::disabled;
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case Power_Class::pmic_t::pmic_m5pm1:
      if (board == board_t::board_M5CoreMatrix)
      {
        return charge_state_t::charging | charge_state_t::not_charging;
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case Power_Class::pmic_t::pmic_m5pm1:
      /// CoreP4X
      return charge_state_t::charging | charge_state_t::not_charging;
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case Power_Class::pmic_t::pmic_ip5306:
      return charge_state_t::charging | charge_state_t::not_charging
           | charge_state_t::full     | charge_state_t::disabled;

    case Power_Class::pmic_t::pmic_axp192:
      /// no full: REG01H bit6 means "not charging or charge complete", which
      /// does not report completion on its own.
      return charge_state_t::charging | charge_state_t::not_charging | charge_state_t::disabled;

#endif

    case Power_Class::pmic_t::pmic_axp2101:
      return charge_state_t::charging    | charge_state_t::not_charging
           | charge_state_t::full        | charge_state_t::disabled
           | charge_state_t::discharging | charge_state_t::idle;

#endif

    default:
      break;
    }

    /// Models whose state is not decided by _pmic. (see getChargeState: a
    /// pmic_m5pm1 case placed above these would make them unreachable without
    /// any diagnostic)
    switch (board)
    {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StickS3:      // PM1 G0
    case board_t::board_M5PaperDIY:     // PM1 G3
    case board_t::board_M5StopWatch:    // PM1 G2
    case board_t::board_M5StampS3Bat:   // PM1 G2
    case board_t::board_M5ChainCaptain: // IOE1 G3
    case board_t::board_M5PaperS3:      // MCU GPIO
    case board_t::board_M5PaperMono:    // IP2315 0xC7 bit7
    case board_t::board_M5PowerHub:     // battery current
      /// no disabled: whether the CHG_EN these boards write is actually wired
      /// to the charger is unconfirmed, and the PaperColor shows it can be
      /// writable and read back while not being connected at all.
      return charge_state_t::charging | charge_state_t::not_charging;

    case board_t::board_M5PaperColor:
      /// only the negative side is answerable: the charger status reaches
      /// neither the PM1 nor the MCU.
      return charge_state_set_t() | charge_state_t::not_charging;
#endif

#if defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
      /// current based: "no charging" is observed as idle, never as not_charging.
      return charge_state_t::charging | charge_state_t::discharging | charge_state_t::idle;
#endif

#if defined (CONFIG_IDF_TARGET_ESP32C5)
    case board_t::board_M5ToughC5:
      return charge_state_t::charging | charge_state_t::not_charging;
#endif

    default:
      break;
    }
    return charge_state_set_t();
  }

  bool Power_Class::getChargeStateCaps(charge_state_set_t* caps)
  {
    if (!_initialized || caps == nullptr) { return false; }
    *caps = _charge_state_caps(_pmic, M5.getBoard());
    return true;
  }

  bool Power_Class::canReport(charge_state_t state)
  {
    charge_state_set_t caps;
    return getChargeStateCaps(&caps) && caps.contains(state);
  }

  std::uint8_t Power_Class::getChargeControlCaps(void)
  {
    if (!_initialized) { return 0; }
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case pmic_t::pmic_aw32001:
      /// no voltage: the driver has it, but it is not wired up. (setChargeVoltage)
      return cap_set_charge_enable | cap_set_charge_current;
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case pmic_t::pmic_m5pm1:
      if (M5.getBoard() == board_t::board_M5CoreMatrix)
      {
        return cap_set_charge_enable | cap_set_charge_current;
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case pmic_t::pmic_m5pm1:
      /// CoreP4X: PM1 CHG_EN only.
      return cap_set_charge_enable;
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_ip5306:
      return cap_set_charge_enable | cap_set_charge_current | cap_set_charge_voltage;

    case pmic_t::pmic_axp192:
      return cap_set_charge_enable | cap_set_charge_current | cap_set_charge_voltage;

#endif

    case pmic_t::pmic_axp2101:
      return cap_set_charge_enable | cap_set_charge_current | cap_set_charge_voltage;

#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case pmic_t::pmic_m5pm1:
      switch (M5.getBoard())
      {
      case board_t::board_M5StampS3Bat:
        return cap_set_charge_enable | cap_set_charge_current;

      case board_t::board_M5PaperColor:
        /// CHG_EN_PP is not connected to the charger.
        return 0;

      case board_t::board_M5StickS3:
      case board_t::board_M5PaperDIY:
      case board_t::board_M5StopWatch:
      case board_t::board_M5ChainCaptain:
      case board_t::board_M5PaperMono:
        return cap_set_charge_enable;

      default:
        break;
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
    case pmic_t::pmic_m5pm1:
      if (M5.getBoard() == board_t::board_M5ToughC5)
      {
        return cap_set_charge_enable | cap_set_charge_current;
      }
      break;
#endif

#endif

    default:
      break;
    }

    switch (M5.getBoard())
    {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
      return cap_set_charge_enable | cap_set_charge_current;
#endif
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5PowerHub:
      return cap_set_charge_enable;
#endif
    default:
      break;
    }
    return 0;
  }

  charge_state_t Power_Class::_getChargeState(void)
  {
    if (!_initialized) { return charge_state_t::not_initialized; }
    if (_identity_unconfirmed) { return charge_state_t::io_error; }   // see begin(): the PMIC never answered its ID probe

    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)

    case pmic_t::pmic_aw32001:
      {
        auto status = Aw32001.getChargeStatus();
        if (status == AW32001_Class::CS_UNKNOWN) { return charge_state_t::io_error; }
        if (status == AW32001_Class::CS_PRE_CHARGE
         || status == AW32001_Class::CS_CHARGE)      { return charge_state_t::charging; }
        if (status == AW32001_Class::CS_CHARGE_DONE) { return charge_state_t::full; }
        bool enabled;
        if (!Aw32001.getBatteryCharge(&enabled)) { return charge_state_t::io_error; }
        return enabled ? charge_state_t::not_charging : charge_state_t::disabled;
      }

#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case pmic_t::pmic_m5pm1:
      /// CoreMatrix: the AW32901 CHG_STAT is wired to IOE1 G8 (low = charging)
      if (M5.getBoard() == board_t::board_M5CoreMatrix)
      {
        return _chargeStateFromChgStat();
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case pmic_t::pmic_m5pm1:
      { /// CoreP4X: the charger status is wired to IOE1 G6 (low = charging).
        /// There is no battery presence signal to gate it with.
        bool level;
        if (!M5.getIOExpander(0).getInputLevel(M5IOE1_Class::gpio6, &level))
        { /// do not report a failed read as "charging"
          return charge_state_t::io_error;
        }
        return level ? charge_state_t::not_charging : charge_state_t::charging;
      }
#else
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    case pmic_t::pmic_ip5306:
      { /// The enable setting (SYS_CTL0 bit4) and its effective value
        /// (REG_READ0 bit3) are different registers: the setting survives an
        /// unplugged supply, the effective one drops with it. Reading both is
        /// what separates "the user disabled it" from "there is no supply".
        /// Limit: the IP5306 has no battery presence signal at all, so a board
        /// running with no cell installed still reports charging.
        bool flag;
        if (!Ip5306.getBatteryCharge(&flag)) { return charge_state_t::io_error; }
        if (!flag) { return charge_state_t::disabled; }
        if (!Ip5306.readChargeActive(&flag)) { return charge_state_t::io_error; }
        if (!flag) { return charge_state_t::not_charging; }
        if (!Ip5306.readChargeFull(&flag)) { return charge_state_t::io_error; }
        return flag ? charge_state_t::full : charge_state_t::charging;
      }

    case pmic_t::pmic_axp192:
      { /// REG00H bit2 is the battery current direction and REG33H bit7 the
        /// charger enable. Completion has no bit of its own here, so full is
        /// never reported. The enable register is only read once the first
        /// step has not decided: a charge in progress does not depend on it.
        bool charging, enabled;
        if (!Axp192.readChargeActive(&charging)) { return charge_state_t::io_error; }
        if (charging) { return charge_state_t::charging; }
        if (!Axp192.getBatteryCharge(&enabled)) { return charge_state_t::io_error; }
        return enabled ? charge_state_t::not_charging : charge_state_t::disabled;
      }

#endif

    case pmic_t::pmic_axp2101:
      { /// One REG01H read carries both the charger state machine (bit[2:0])
        /// and the battery current direction (bit[6:5]).
        /// Order matters: the charger is asked first, and only once it says it
        /// is not charging does the current direction narrow the answer down.
        std::uint8_t status;
        if (!Axp2101.readPmuStatus2(&status)) { return charge_state_t::io_error; }
        std::uint8_t machine = status & 0x07;
        /// 0b000-0b011 = trickle / pre / constant current / constant voltage
        if (machine <= 0x03) { return charge_state_t::charging; }
        /// 0b100 = charge done. (disabling the charger clears it, so this does
        /// not shadow the disabled branch below)
        if (machine == 0x04) { return charge_state_t::full; }
        bool enabled;
        if (!Axp2101.getBatteryCharge(&enabled)) { return charge_state_t::io_error; }
        if (!enabled) { return charge_state_t::disabled; }
        switch (status & 0x60)
        {
        case 0x40: return charge_state_t::discharging;
        case 0x00: return charge_state_t::idle;
        default:   break;
        }
        return charge_state_t::not_charging;
      }

#endif

    default:
      break;
    }

    switch (M5.getBoard()) {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5PaperMono:
      {
        // No external power -> not charging. PWR_SRC is a bitmap, and the battery bit may coexist with VIN.
        M5PM1_Class::pwr_src_t sources;
        if (!M5pm1.getPowerSource(&sources)) { return charge_state_t::io_error; }
        if (!(sources & (M5PM1_Class::vin | M5PM1_Class::vinout))) { return charge_state_t::not_charging; }
        // External power present. The IP2316 charger reports
        // its state in REG_CHG_STAT(0xC7): bit7 = charging in progress (measured:
        // 0x82 charging / 0x45 charge-complete / 0x00 charge-disabled).
        // The other bits carry meaning too, but their definition is unconfirmed,
        // so only bit7 is used and full / disabled are not reported.
        /// a charger that does not answer through the gate is an unfinished
        /// procedure, not missing evidence: io_error, not undetermined. The
        /// gate writes themselves are part of the procedure, so a failed open
        /// or close is io_error as well.
        charge_state_t res = charge_state_t::io_error;
        uint8_t chg_stat;
        if (set_papermono_ip2315_enabled(true)
         && wait_papermono_ip2315_ready()
         && M5.In_I2C.readRegister(ip2315_i2c_addr, 0xC7, &chg_stat, 1, i2c_freq))
        {
          res = (chg_stat & (1 << 7)) ? charge_state_t::charging : charge_state_t::not_charging;
        }
        if (!set_papermono_ip2315_enabled(false)) { res = charge_state_t::io_error; }
        return res;
      }

      case board_t::board_M5PaperColor:
      { /// The charger status is wired to neither the PM1 nor the MCU, so only
        /// the negative side can be answered: with no external power there can
        /// be no charging. Anything else stays undetermined.
        M5PM1_Class::pwr_src_t sources;
        if (!M5pm1.getPowerSource(&sources)) { return charge_state_t::io_error; }
        if (!(sources & (M5PM1_Class::vin | M5PM1_Class::vinout))) { return charge_state_t::not_charging; }
        return charge_state_t::undetermined;
      }

      case board_t::board_M5StickS3:     // PM1 G0
      case board_t::board_M5PaperDIY:    // PM1 G3
      case board_t::board_M5StopWatch:   // PM1 G2
      case board_t::board_M5StampS3Bat:  // PM1 G2
      { /// CHG_STAT on a PM1 GPIO, low = charging.
        auto pin = (M5.getBoard() == board_t::board_M5StickS3)  ? M5PM1_Class::gpio0
                 : (M5.getBoard() == board_t::board_M5PaperDIY) ? M5PM1_Class::gpio3
                 : M5PM1_Class::gpio2;
        std::uint8_t bits;
        /// getGPIOInput() folds a failed read into "high"; read the whole
        /// register instead so that a failure is not reported as not_charging.
        if (!M5pm1.getGPIOInputBits(&bits)) { return charge_state_t::io_error; }
        return (bits & (1 << (std::uint8_t)pin)) ? charge_state_t::not_charging : charge_state_t::charging;
      }

      case board_t::board_M5ChainCaptain:
      { /// CHG_STAT is on IOE1 G3, low = charging.
        bool level;
        if (!M5.getIOExpander(0).getInputLevel(M5IOE1_Class::gpio3, &level))
        {
          return charge_state_t::io_error;
        }
        return level ? charge_state_t::not_charging : charge_state_t::charging;
      }

      case board_t::board_M5PaperS3:
        /// CHG_STAT is wired to an MCU pin, so there is no failing read here:
        /// this is the only model that never answers io_error.
        return (m5gfx::gpio_in(M5PaperS3_CHG_STAT_PIN) == false)
             ? charge_state_t::charging : charge_state_t::not_charging;

      case board_t::board_M5PowerHub:
      { /// battery current over a threshold. The accuracy of this reading is
        /// not trusted, so the sign is not used to tell discharging from idle.
        std::int32_t mA;
        if (!_readBatteryCurrent(&mA)) { return charge_state_t::io_error; }
        return (mA > 10) ? charge_state_t::charging : charge_state_t::not_charging;
      }
#endif
#if defined (CONFIG_IDF_TARGET_ESP32P4)
      case board_t::board_M5Tab5:
      case board_t::board_M5Tab5X:
      { /// The INA226 battery current decides the state.
        /// IOE1 G6 is deliberately not used: it is the IP2326 BAT_STAT, which
        /// only marks the trickle / constant-current stage and reads high both
        /// while charging and while charging is disabled.
        /// not_charging is never reported here - a stopped charge is observed
        /// as idle.
        std::int32_t mA;
        if (!_readBatteryCurrent(&mA)) { return charge_state_t::io_error; }
        static constexpr std::int32_t threshold_mA = 10;
        if (mA >  threshold_mA) { return charge_state_t::charging; }
        if (mA < -threshold_mA) { return charge_state_t::discharging; }
        return charge_state_t::idle;
      }
#endif
#if defined (CONFIG_IDF_TARGET_ESP32C5)
      case board_t::board_M5ToughC5:
        // The LGS4056 CHG_STAT is wired to IOE1 G3, low=charging / high=not charging.
        // Near full charge it alternates with the charger's top-off cycle (~10-20s).
        return _chargeStateFromChgStat();
#endif
      default:
        break;
    }
    return charge_state_t::unsupported;
  }

  charge_state_t Power_Class::getChargeState(void)
  {
    charge_state_t res = _getChargeState();
#if defined (M5UNIFIED_PC_BUILD) || defined (M5UNIFIED_CHECK_CHARGE_STATE_CAPS)
    /// The procedure and the capability table are written separately, so the
    /// two are cross checked where a failing check can actually be seen: a
    /// state that is not advertised would silently break every caller that
    /// asked getChargeStateCaps() what to expect.
    assert(!charge_states_known.contains(res)
        || _charge_state_caps(_pmic, M5.getBoard()).contains(res));
#endif
    return res;
  }

  bool Power_Class::isCharging(void)
  {
    return charge_states_any_charging.contains(getChargeState());
  }

  battery_presence_t Power_Class::getBatteryPresence(void)
  {
    if (!_initialized) { return battery_presence_t::not_initialized; }
    if (_identity_unconfirmed) { return battery_presence_t::io_error; }   // see begin(): the PMIC never answered its ID probe

    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case pmic_t::pmic_m5pm1:
      if (M5.getBoard() == board_t::board_M5CoreMatrix)
      { /// there is no presence signal: it is inferred, and stays undetermined
        /// until the first verdict. (see _batteryPresent) The verdict is
        /// cached across failed reads, so the bus is probed first: a dead bus
        /// is io_error, not the last verdict.
        bool io_ok;
        std::int8_t bp = _batteryPresent(&io_ok);
        if (!io_ok) { return battery_presence_t::io_error; }
        return (bp < 0)  ? battery_presence_t::undetermined
             : (bp == 0) ? battery_presence_t::absent
                         : battery_presence_t::present;
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
#else

    case pmic_t::pmic_axp2101:
      { /// REG00H bit3 follows an attach / detach.
        std::uint8_t status;
        if (!Axp2101.readPmuStatus1(&status)) { return battery_presence_t::io_error; }
        return (status & 0x08) ? battery_presence_t::present : battery_presence_t::absent;
      }

#if defined (CONFIG_IDF_TARGET_ESP32C5)
    case pmic_t::pmic_m5pm1:
      if (M5.getBoard() == board_t::board_M5ToughC5)
      { /// see the CoreMatrix note above.
        bool io_ok;
        std::int8_t bp = _batteryPresent(&io_ok);
        if (!io_ok) { return battery_presence_t::io_error; }
        return (bp < 0)  ? battery_presence_t::undetermined
             : (bp == 0) ? battery_presence_t::absent
                         : battery_presence_t::present;
      }
      break;
#endif

#endif

    default:
      break;
    }
    return battery_presence_t::unsupported;
  }

  float Power_Class::_readExtValue(ext_port_mask_t port_mask, bool is_voltage)
  {
#if defined(M5UNIFIED_PC_BUILD)
      (void)port_mask;
      (void)is_voltage;
#else
    switch (M5.getBoard()) {
    #if defined(CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5PowerHub: {
        struct PortReg {
          ext_port_mask_t mask;
          uint8_t reg;
        };
        static const PortReg port_regs[] = {
          {ext_port_mask_t::ext_PA, 0x40},
          {ext_port_mask_t::ext_PC1, 0x44},
          {ext_port_mask_t::ext_USB, 0x3C},
          {ext_port_mask_t::ext_PWR485, 0x38},
          {ext_port_mask_t::ext_PWRCAN, 0x34},
        };

        uint8_t buf[2];
        for (const auto& pr : port_regs) {
          if (port_mask & pr.mask) {
            if (M5.In_I2C.readRegister(powerhub_i2c_addr, pr.reg + (is_voltage ? 0 : 2), buf, sizeof(buf), i2c_freq)) {
              return (int16_t)((buf[1] << 8) | buf[0]);
            }
              return 0;
          }
        }
        return 0;
      }

      case board_t::board_M5StampS3Bat:
      case board_t::board_M5StopWatch:
      case board_t::board_M5StickS3: {
        return M5pm1.get5VoutVoltage();
      } break;

      case board_t::board_M5ChainCaptain: {
        if (!is_voltage) { return 0; }
        static constexpr float diode_offset_mv = 530.0f;
        static constexpr float valid_voltage_threshold_mv = 2000.0f;
        if (port_mask & ext_port_mask_t::ext_PA) {
          float mv = M5pm1.getVBUSVoltage();
          return mv >= valid_voltage_threshold_mv ? mv + diode_offset_mv : mv;
        }
        if (port_mask & (ext_port_mask_t::ext_PB1 | ext_port_mask_t::ext_PB2)) {
          float mv = M5pm1.get5VoutVoltage();
          return mv >= valid_voltage_threshold_mv ? mv + diode_offset_mv : mv;
        }
        return 0;
      }

      case board_t::board_M5StampPLC:
        if (port_mask & (ext_port_mask_t::ext_PWR485 | ext_port_mask_t::ext_PWRCAN)) {
          if (is_voltage)
            return Ina226.getBusVoltage() * 1000;
          else
            return Ina226.getShuntCurrent() * 1000;
        }
        return 0;
    #endif
      default:
        return 0;
      }
#endif
    return 0;
  }

  float Power_Class::getExtVoltage(ext_port_mask_t port_mask)
  {
    return _readExtValue(port_mask, true);
  }

  float Power_Class::getExtCurrent(ext_port_mask_t port_mask)
  {
    return _readExtValue(port_mask, false);
  }

  uint8_t Power_Class::getKeyState(void)
  {
    switch (_pmic)
    {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case pmic_t::pmic_axp2101:
      return Axp2101.getPekPress();

    case pmic_t::pmic_m5pm1:
      return M5pm1.getPekPress();

#elif defined (CONFIG_IDF_TARGET_ESP32C61) || defined (CONFIG_IDF_TARGET_ESP32C5)
    case pmic_t::pmic_m5pm1:
      return M5pm1.getPekPress();

#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case pmic_t::pmic_m5pm1:
      return M5pm1.getPekPress();

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case pmic_t::pmic_axp192:
      return Axp192.getPekPress();

    case pmic_t::pmic_axp2101:
      return Axp2101.getPekPress();
#endif

    default:
      return 0;
    }
  }

  void Power_Class::setExtPortBusConfig(const ext_port_bus_t& config)
  {
    switch (M5.getBoard()) {
    #if defined(CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5PowerHub: {
        uint8_t buf[5];
        buf[0] = config.voltage & 0xFF;
        buf[1] = config.voltage >> 8;
        buf[2] = config.currentLimit & 0xFF;
        buf[3] = config.enable;
        buf[4] = config.direction;
        M5.In_I2C.writeRegister(powerhub_i2c_addr, 0x20, buf, sizeof(buf), i2c_freq);
      } break;
    #endif
      default:
        break;
    }
  }


  void Power_Class::setVibration(uint8_t level)
  {
#if !defined (M5UNIFIED_PC_BUILD) && defined (CONFIG_IDF_TARGET_ESP32S3)
    if (M5.getBoard() == board_t::board_M5StopWatch)
    {
      // M5IOE1 PWM1 (0x1B/0x1C) -> pin IO9 / G9 motor; duty 12-bit in [11:0], EN=bit7 of high byte.
      auto& ioe1 = static_cast<M5IOE1_Class&>(M5.getIOExpander(0));
      if (level == 0) {
        ioe1.setPwmDuty12bit(M5IOE1_Class::pwm_ch1, 0, pwm_polarity_t::normal, false);
      } else {
        // PWM needs IO9 in output mode (M5IOE1 pin index 8 -> GPIO_MODE_H bit0).
        ioe1.setHighImpedance(M5IOE1_Class::gpio9, false);
        ioe1.setDirection(M5IOE1_Class::gpio9, true);
        uint16_t duty12 = static_cast<uint16_t>((static_cast<uint32_t>(level) * 0x0FFFu) / 255u);
        ioe1.setPwmDuty12bit(M5IOE1_Class::pwm_ch1, duty12);
      }
      return;
    }
#endif
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
