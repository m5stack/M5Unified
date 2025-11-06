// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_SOFT_I2C_CLASS_H__
#define __M5_SOFT_I2C_CLASS_H__

#include <driver/gpio.h>
#include <cstdint>
#include <cstddef>
#include "I2C_Class.hpp"

namespace m5
{
  class SoftI2C_Class : public I2C_Class
  {
  public:
    /// Constructor
    SoftI2C_Class();
    
    /// Destructor
    ~SoftI2C_Class();

    /// setup I2C pin parameters. (No begin)
    /// @param pin_sda SDA pin number.
    /// @param pin_scl SCL pin number.
  void setPins(int pin_sda, int pin_scl);

    /// setup and begin I2C peripheral. (No communication is performed.)
    /// @param port_num Invalid parameters. In this SoftI2C implementation, this parameter is ignored. default I2C_NUM_0.
    /// @param pin_sda SDA pin number.
    /// @param pin_scl SCL pin number.
    /// @return success(true) or failed(false).
  bool begin(i2c_port_t port_num, int pin_sda, int pin_scl) override;

    /// begin I2C peripheral. (No communication is performed.)
    /// @return success(true) or failed(false).
  bool begin(void) override;

    /// release I2C peripheral.
    /// @return success(true) or failed(false).
  bool release(void) const override;

    /// Sends the I2C start condition and the address of the slave.
    /// @param address slave addr.
    /// @param read bit of read flag. true=read / false=write.
    /// @return success(true) or failed(false).
  bool start(std::uint8_t address, bool read, std::uint32_t freq) const override;

    /// Sends the I2C repeated start condition and the address of the slave.
    /// @param address slave addr.
    /// @param read bit of read flag. true=read / false=write.
    /// @return success(true) or failed(false).
  bool restart(std::uint8_t address, bool read, std::uint32_t freq) const override;

    /// Sends the I2C stop condition.
    /// If an ACK error occurs, return false.
    /// @return success(true) or failed(false).
  bool stop(void) const override;

    /// Send 1 byte of data.
    /// @param data write data.
    /// @return success(true) or failed(false).
  bool write(std::uint8_t data) const override;

    /// Send multiple bytes of data.
    /// @param[in] data write data array.
    /// @param     length data array length.
    /// @return success(true) or failed(false).
  bool write(const std::uint8_t* data, std::size_t length) const override;

    /// Receive multiple bytes of data.
    /// @param[out] result read data array.
    /// @param      length data array length.
    /// @return success(true) or failed(false).
  bool read(std::uint8_t* result, std::size_t length, bool last_nack = false) const override;

    //----------

    /// Write multiple bytes value to the register. Performs a series of communications from START to STOP.
    /// @param address slave addr.
    /// @param reg     register number.
    /// @param[in] data write data array.
    /// @param     length data array length.
    /// @return success(true) or failed(false).
  bool writeRegister(std::uint8_t address, std::uint8_t reg, const std::uint8_t* data, std::size_t length, std::uint32_t freq) const override;

    /// Read multiple bytes value from the register. Performs a series of communications from START to STOP.
    /// @param address slave addr.
    /// @param reg     register number.
    /// @param[out] result read data array.
    /// @param      length data array length.
    /// @return success(true) or failed(false).
  bool readRegister(std::uint8_t address, std::uint8_t reg, std::uint8_t* result, std::size_t length, std::uint32_t freq) const override;

    /// Write a 1-byte value to the register. Performs a series of communications from START to STOP.
    /// @param address slave addr.
    /// @param reg     register number.
    /// @param data    write data.
    /// @return success(true) or failed(false).
  bool writeRegister8(std::uint8_t address, std::uint8_t reg, std::uint8_t data, std::uint32_t freq) const override;

    /// Read a 1-byte value from the register. Performs a series of communications from START to STOP.
    /// @param address slave addr.
    /// @param reg     register number.
    /// @return read value.
  std::uint8_t readRegister8(std::uint8_t address, std::uint8_t reg, std::uint32_t freq) const override;

    /// Write a 1-byte value to the register by bit add operation. Performs a series of communications from START to STOP.
    /// @param address slave addr.
    /// @param reg     register number.
    /// @param data    add bit data.
    /// @return success(true) or failed(false).
  bool bitOn(std::uint8_t address, std::uint8_t reg, std::uint8_t data, std::uint32_t freq) const override;

    /// Write a 1-byte value to the register by bit erase operation. Performs a series of communications from START to STOP.
    /// @param address slave addr.
    /// @param reg     register number.
    /// @param data    erase bit data.
    /// @return success(true) or failed(false).
  bool bitOff(std::uint8_t address, std::uint8_t reg, std::uint8_t data, std::uint32_t freq) const override;

    /// execute I2C scan. (for 7bit address)
    /// @param[out] result data array needs 120 Bytes.
    void scanID(bool* result, std::uint32_t freq = 100000) const;

    bool scanID(uint8_t addr, std::uint32_t freq = 100000) const;

    int8_t getSDA(void) const { return _pin_sda; }
    int8_t getSCL(void) const { return _pin_scl; }
    uint32_t getFrequency(void) const { return _freq; }

    bool isEnabled(void) const { return _pin_sda >= 0 && _pin_scl >= 0; }

  private:
    int8_t _pin_sda = -1;
    int8_t _pin_scl = -1;
    uint32_t _freq = 100000;
    mutable uint32_t _delay_us = 5;

    void _delay() const;
    void _scl_high() const;
    void _scl_low() const;
    void _sda_high() const;
    void _sda_low() const;
    bool _sda_read() const;
    bool _i2c_start() const;
    bool _i2c_stop() const;
    bool _i2c_write_byte(uint8_t data) const;
    uint8_t _i2c_read_byte(bool ack) const;
  };

  /// for internal I2C device
  extern SoftI2C_Class In_SoftI2C;
}

#endif
