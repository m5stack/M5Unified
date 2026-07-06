// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_M5PM1_CLASS_H__
#define __M5_M5PM1_CLASS_H__

#include "../I2C_Class.hpp"

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
    , wake    = 0b10
    , special = 0b11
    };

    /// PM1 GPIO output driver type.
    enum gpio_drive_t : std::uint8_t
    { push_pull  = 0
    , open_drain = 1
    };

    /// PM1 power source status.
    enum pwr_src_t : std::uint8_t
    { vin     = 0
    , vinout  = 1
    , battery = 2
    , unknown = 3
    };

    /// set BOOST/Grove 5V output enable.
    /// @param enable true=enable / false=disable
    bool setExtOutput(bool enable);

    /// get BOOST/Grove 5V output enable setting.
    bool getExtOutput(void);

    /// set PM1 LDO output enable.
    /// @param enable true=enable / false=disable
    bool setLDOOutput(bool enable);

    /// get current PM1 power source.
    pwr_src_t getPowerSource(void);

    /// set PM1 GPIO mux function.
    bool setGPIOFunction(gpio_t pin, gpio_function_t function);

    /// set PM1 GPIO direction.
    bool setGPIOMode(gpio_t pin, gpio_mode_t mode);

    /// set PM1 GPIO output driver type.
    bool setGPIODrive(gpio_t pin, gpio_drive_t drive);

    /// set PM1 GPIO output latch level.
    bool setGPIOOutput(gpio_t pin, bool high);

    /// get PM1 GPIO input level.
    bool getGPIOInput(gpio_t pin);

    /// get PM1 GPIO output latch level, not the physical input level.
    bool getGPIOOutputLatch(gpio_t pin);

    /// set battery charge enable.
    /// @param enable true=enable / false=disable
    bool setBatteryCharge(bool enable);

    /// set battery charge current
    /// @param max_mA milli ampere. (8 - 512).
    bool setChargeCurrent(std::uint16_t max_mA);

    /// set battery charge voltage
    /// @param max_mV milli volt. (3600 - 4545).
    bool setChargeVoltage(std::uint16_t max_mV);

    /// Get whether the battery is currently charging or not.
    bool isCharging(void);

    // get setting value of battery charge current
    /// @return milli ampere. (8 - 512). 0=unknown
    std::uint16_t getChargeCurrent(void);

    // get setting value of battery charge voltage
    /// @return milli volt. (3600 - 4545). 0=unknown
    std::uint16_t getChargeVoltage(void);

    /// Get power key press condition.
    /// @return 0=none / 2=short clicked
    uint8_t getPekPress(void);

    /// get VIN voltage.
    /// @return milli volt. 0=read failed
    std::uint16_t getVBUSVoltage(void);

    /// get battery voltage.
    /// @return milli volt. 0=read failed
    std::uint16_t getBatteryVoltage(void);

    /// get 5V output voltage.
    /// @return milli volt. 0=read failed
    std::uint16_t get5VoutVoltage(void);

    /// power off PM1.
    bool powerOff(void);
  };
}

#endif
