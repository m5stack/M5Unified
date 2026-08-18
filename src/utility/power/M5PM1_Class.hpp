// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_M5PM1_CLASS_H__
#define __M5_M5PM1_CLASS_H__

#include "../I2C_Class.hpp"
#include "../pwm_types.hpp"

namespace m5
{
  class M5PM1_Class : public I2C_Device
  {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x6E;

    M5PM1_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 100000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    bool begin(void);

    /// PM1 GPIO pin number.
    enum gpio_t : std::uint8_t
    { gpio0 = 0
    , gpio1 = 1
    , gpio2 = 2
    , gpio3 = 3
    , gpio4 = 4
    };

    /// PM1 GPIO direction.
    enum gpio_mode_t : std::uint8_t
    { input  = 0
    , output = 1
    };

    /// PM1 GPIO mux function. "special" depends on the pin (ADC/PWM/LED).
    enum gpio_function_t : std::uint8_t
    { gpio    = 0b00
    , irq     = 0b01
      // 0b10 is reserved in the datasheet.
    , special = 0b11
    };

    /// PM1 GPIO pull-up/pull-down setting.
    enum gpio_pull_t : std::uint8_t
    { pull_none = 0b00
    , pull_up   = 0b01
    , pull_down = 0b10
    };

    /// PM1 GPIO output driver type.
    enum gpio_drive_t : std::uint8_t
    { push_pull  = 0
    , open_drain = 1
    };

    /// PM1 PWM channel. Both channels share the same frequency setting.
    enum pwm_channel_t : std::uint8_t
    { pwm_ch0 = 0 // GPIO3; may be assigned to another function depending on the board.
    , pwm_ch1 = 1 // GPIO4
    };

    /// PM1 power source bitmap. Multiple values may be combined.
    enum pwr_src_t : std::uint8_t
    { none    = 0
    , vin     = 1 << 0
    , vinout  = 1 << 1
    , battery = 1 << 2
    };

    /// set BOOST/Grove 5V output enable.
    /// @param enable true=enable / false=disable
    bool setExtOutput(bool enable);

    /// get BOOST/Grove 5V output enable setting.
    bool getExtOutput(void);

    /// set PM1 LDO output enable.
    /// @param enable true=enable / false=disable
    bool setLDOOutput(bool enable);

    /// set PM1 3.3V DCDC rail output enable (PWR_CFG bit1 = 3.3V_DCDC_EN).
    /// @param enable true=enable / false=disable
    bool setDCDCOutput(bool enable);

    /// set the default level of the PM1 LED_EN pin.
    /// @param level true=high / false=low
    bool setLedEnLevel(bool level);

    /// get the PM1 PWR_SRC bitmap.
    /// bit0=5VIN valid, bit1=5VINOUT valid (0 while the 5V boost is enabled),
    /// bit2=VBAT node valid. Multiple sources may be present simultaneously.
    pwr_src_t getPowerSource(void);

    /// get whether PWR_SRC reports the VBAT node rail as powered.
    /// note: this tracks the node voltage, not physical battery presence.
    bool getVbatNodePowered(bool* powered);

    /// set PM1 GPIO mux function.
    bool setGPIOFunction(gpio_t pin, gpio_function_t function);

    /// set PM1 GPIO direction.
    bool setGPIOMode(gpio_t pin, gpio_mode_t mode);

    /// set PM1 GPIO pull-up/pull-down setting.
    bool setGPIOPull(gpio_t pin, gpio_pull_t pull);

    /// set PM1 GPIO output driver type.
    bool setGPIODrive(gpio_t pin, gpio_drive_t drive);

    /// set PM1 GPIO output latch level.
    bool setGPIOOutput(gpio_t pin, bool high);

    /// get PM1 GPIO input level.
    bool getGPIOInput(gpio_t pin);

    /// read all GPIO input levels at once. returns false on I2C failure.
    bool getGPIOInputBits(std::uint8_t* bits);

    /// get PM1 GPIO output latch level, not the physical input level.
    bool getGPIOOutputLatch(gpio_t pin);

    /// set the PWM frequency in Hz.
    /// @note The frequency is shared by both PWM channels, so changing it also
    /// changes a channel that is already running.
    /// @note PWM channel 0 maps to GPIO3 and channel 1 maps to GPIO4. Call
    /// setGPIOFunction() with special separately to route PWM to the pin.
    bool setPwmFrequency(std::uint16_t frequency);

    /// set PWM duty in percent.
    /// @param channel PWM channel (pwm_ch0 / pwm_ch1).
    /// @param duty duty cycle in percent (0-100).
    /// @param polarity PWM output polarity.
    /// @param enable true=enable / false=disable.
    /// @note PWM channel 0 maps to GPIO3 and channel 1 maps to GPIO4. Call
    /// setGPIOFunction() with special separately to route PWM to the pin.
    bool setPwmDutyPercent(pwm_channel_t channel, std::uint32_t duty,
                           pwm_polarity_t polarity = pwm_polarity_t::normal, bool enable = true);

    /// set PWM duty with 12-bit precision.
    /// @param channel PWM channel (pwm_ch0 / pwm_ch1).
    /// @param duty12 duty cycle (0-4095).
    /// @param polarity PWM output polarity.
    /// @param enable true=enable / false=disable.
    /// @note PWM channel 0 maps to GPIO3 and channel 1 maps to GPIO4. Call
    /// setGPIOFunction() with special separately to route PWM to the pin.
    bool setPwmDuty12bit(pwm_channel_t channel, std::uint32_t duty12,
                         pwm_polarity_t polarity = pwm_polarity_t::normal, bool enable = true);

    /// clear PM1 wake source bits selected by mask.
    /// Single-write selective clear assuming the write-zero-to-clear behavior
    /// adopted by the official driver; the datasheet does not specify the write polarity.
    /// Bits outside [6:0] are written as zero, matching the full-clear precedent.
    bool clearWakeSource(std::uint8_t mask = 0x7F);

    /// clear all PM1 GPIO IRQ status bits.
    bool clearGPIOIRQStatus(void);

    /// clear all PM1 system IRQ status bits.
    bool clearSystemIRQStatus(void);

    /// clear all PM1 button IRQ status bits.
    bool clearButtonIRQStatus(void);

    /// clear all PM1 IRQ status bits.
    bool clearIRQStatus(void);

    /// set PM1 GPIO IRQ mask register. bit=1 disables interrupt.
    bool setGPIOIRQMaskBits(std::uint8_t mask);

    /// set PM1 system IRQ mask register. bit=1 disables interrupt.
    bool setSystemIRQMaskBits(std::uint8_t mask);

    /// set PM1 button IRQ mask register. bit=1 disables interrupt.
    bool setButtonIRQMaskBits(std::uint8_t mask);

    /// set battery charge enable.
    /// @param enable true=enable / false=disable
    bool setBatteryCharge(bool enable);

    /// get battery charge enable state with I2C error reporting.
    /// @param enabled output parameter, receives the charge enable state.
    /// @return false on I2C failure.
    bool getBatteryCharge(bool* enabled);

    /// set battery charge current
    /// @param max_mA ignored; the PM1 has no charge current register.
    /// @note The PM1 register map exposes no charge current register; this is a permanent stub returning false.
    bool setChargeCurrent(std::uint16_t max_mA);

    /// set battery charge voltage
    /// @param max_mV ignored; the PM1 has no charge voltage register.
    /// @note The PM1 register map exposes no charge voltage register; this is a permanent stub returning false.
    bool setChargeVoltage(std::uint16_t max_mV);

    /// Get whether the battery is currently charging or not.
    /// @note The PM1 register map exposes no charging status register; this is a permanent stub returning false.
    bool isCharging(void);

    // get setting value of battery charge current
    /// @return always 0.
    /// @note The PM1 register map exposes no charge current register; this is a permanent stub returning 0.
    std::uint16_t getChargeCurrent(void);

    // get setting value of battery charge voltage
    /// @return always 0.
    /// @note The PM1 register map exposes no charge voltage register; this is a permanent stub returning 0.
    std::uint16_t getChargeVoltage(void);

    /// Get power key press condition.
    /// @return 0=none / 2=short clicked. For AXP compatibility, a double click
    /// also reports 2; use wasPekDoubleClicked() to distinguish it.
    /// Only the consumed click flags are cleared; the WAKEUP flag is preserved.
    /// Returns 0 and leaves the event pending if the clear write fails.
    uint8_t getPekPress(void);

    /// Returns whether the most recently reported click (getPekPress() == 2)
    /// was a double click, then clears the flag.
    /// Call from the task that polls getPekPress() (typically right after
    /// M5.update()); the flag is not synchronized across tasks.
    bool wasPekDoubleClicked(void);

    /// get VIN voltage.
    /// @return milli volt. 0=read failed
    std::uint16_t getVBUSVoltage(void);

    /// get battery voltage.
    /// @return milli volt. 0=read failed
    std::uint16_t getBatteryVoltage(void);

    /// get battery voltage with I2C error reporting.
    /// @param millivolt output parameter, receives the battery voltage [mV].
    /// @return false on I2C failure.
    bool getBatteryVoltage(std::uint16_t* millivolt);

    /// get 5V output voltage.
    /// @return milli volt. 0=read failed
    std::uint16_t get5VoutVoltage(void);

    /// power off PM1.
    bool powerOff(void);

  private:
    bool _pek_double_pending = false;
  };
}

#endif
