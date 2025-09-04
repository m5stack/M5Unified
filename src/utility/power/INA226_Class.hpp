// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_INA226_CLASS_H__
#define __M5_INA226_CLASS_H__

#include "../I2C_Class.hpp"

namespace m5
{
  class INA226_Class : public I2C_Device
  {
  public:
    static constexpr std::uint8_t INA226_CONFIG = 0x00;
    static constexpr std::uint8_t INA226_SHUNT_V = 0x01;
    static constexpr std::uint8_t INA226_BUS_V = 0x02;
    static constexpr std::uint8_t INA226_POWER = 0x03;
    static constexpr std::uint8_t INA226_CURRENT = 0x04;
    static constexpr std::uint8_t INA226_CALIBRATION = 0x05;
    static constexpr std::uint8_t INA226_MASK_ENABLE = 0x06;
    static constexpr std::uint8_t INA226_ALERT_LIMIT = 0x07;

    static constexpr std::uint16_t INA226_BIT_SOL  = 0x8000;
    static constexpr std::uint16_t INA226_BIT_SUL  = 0x4000;
    static constexpr std::uint16_t INA226_BIT_BOL  = 0x2000;
    static constexpr std::uint16_t INA226_BIT_BUL  = 0x1000;
    static constexpr std::uint16_t INA226_BIT_POL  = 0x0800;
    static constexpr std::uint16_t INA226_BIT_CNVR = 0x0400;
    static constexpr std::uint16_t INA226_BIT_AFF  = 0x0010;
    static constexpr std::uint16_t INA226_BIT_CVRF = 0x0008;
    static constexpr std::uint16_t INA226_BIT_OVF  = 0x0004;
    static constexpr std::uint16_t INA226_BIT_APOL = 0x0002;
    static constexpr std::uint16_t INA226_BIT_LEN  = 0x0001;

    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x40;

    enum class Sampling : uint8_t {
      Rate1 = 0b000,     //!< 1 sps (as default)
      Rate4 = 0b001,     //!< 4 sps
      Rate16 = 0b010,    //!< 16 sps
      Rate64 = 0b011,    //!< 64 sps
      Rate128 = 0b100,   //!< 128 sps
      Rate256 = 0b101,   //!< 256 sps
      Rate512 = 0b110,   //!< 512 sps
      Rate1024 = 0b111,  //!< 1024 sps
    };

    /*!
    @enum ConversionTime
    @brief The conversion time for the bus voltage measurement
    */
    enum class ConversionTime : uint8_t {
      US_140 = 0b000,   //!< 140 us
      US_204 = 0b001,   //!< 204 us
      US_332 = 0b010,   //!< 332 us
      US_588 = 0b011,   //!< 588 us
      US_1100 = 0b100,  //!< 1.1 ms (as default)
      US_2116 = 0b101,  //!< 2.116 ms
      US_4156 = 0b110,  //!< 4.156 ms
      US_8244 = 0b111,  //!< 8.244 ms
    };

    /*!
    @enum Mode
    @brief Operation mode
    */
    enum class Mode : uint8_t {
      PowerDown = 0b000,           //!< Power-Down (or Shutdown)
      ShuntVoltageSingle = 0b001,  //!< Shunt Voltage, Triggered
      BusVoltageSingle = 0b010,    //!< Bus Voltage, Triggered
      ShuntAndBusSingle = 0b011,   //!< Shunt and Bus, Triggered
      // 0x04 Same as PowerDown
      ShuntVoltage = 0b101,  //!< Shunt Voltage, Continuous
      BusVoltage = 0b110,           //!< Bus Voltage, Continuous
      ShuntAndBus = 0b111,          //!< Shunt and Bus, Continuous (as default)
    };

    /*!
    @enum Alert
    @brief Alert type
    */
    enum class Alert : int8_t {
      Unknown = -1,
      None,             //!< No alert
      ShuntOver,        //!< Shunt Voltage Over-Voltage
      ShuntUnder,       //!< Shunt Voltage Under-Voltage
      BusOver,          //!< Bus Voltage Over-Voltage
      BusUnder,         //!< Bus Voltage Under-Voltage
      PowerOver,        //!< Power Over-limit
      ConversionReady,  //!< Conversion Ready
    };

    struct config_t {
      float shunt_res = 0.1f;
      float max_expected_current = 2.0f;
      Sampling sampling_rate = Sampling::Rate16;
      ConversionTime shunt_conversion_time = ConversionTime::US_1100;
      ConversionTime bus_conversion_time = ConversionTime::US_1100;
      Mode mode = Mode::ShuntAndBus;
    };

    INA226_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {
      _shunt_res = 10;
    }

    bool begin(void);

    void config(const config_t& cfg);

    float getBusVoltage(void);
    float getShuntVoltage(void);
    float getShuntCurrent(void);
    float getPower(void);

  private:
    std::size_t readRegister16(std::uint8_t addr);
    bool writeRegister16(std::uint8_t addr, std::uint16_t data);

    float _shunt_res = 0.1f;
    float _cur_lsb = 0.1f;
  };
}

#endif
