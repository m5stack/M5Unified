// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "SoftI2C_Class.hpp"
#include <cstring>
#include "driver/gpio.h"
#include "esp_rom_sys.h"


namespace m5
{
  SoftI2C_Class In_SoftI2C;

  SoftI2C_Class::SoftI2C_Class()
  {
    _pin_sda = -1;
    _pin_scl = -1;
    _freq = 100000; // default 100kHz
    _delay_us = 5;
  }

  SoftI2C_Class::~SoftI2C_Class()
  {
    release();
  }

  void SoftI2C_Class::setPins(int pin_sda, int pin_scl)
  {
    _pin_sda = pin_sda;
    _pin_scl = pin_scl;
  }

  bool SoftI2C_Class::begin(i2c_port_t port_num,int pin_sda, int pin_scl)
  {
    setPins(pin_sda, pin_scl);
    return begin();
  }

  bool SoftI2C_Class::begin(void)
  {
    if (_pin_sda < 0 || _pin_scl < 0) {
      return false;
    }

    gpio_config_t scl_conf = {};
    scl_conf.pin_bit_mask = (1ULL << _pin_scl);
    scl_conf.mode = GPIO_MODE_OUTPUT_OD;
    scl_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    scl_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    scl_conf.intr_type = GPIO_INTR_DISABLE;
    esp_err_t ret = gpio_config(&scl_conf);
    if (ret != ESP_OK) {
      return false;
    }

    gpio_config_t sda_conf = {};
    sda_conf.pin_bit_mask = (1ULL << _pin_sda);
    sda_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    sda_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    sda_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    sda_conf.intr_type = GPIO_INTR_DISABLE;
    ret = gpio_config(&sda_conf);
    if (ret != ESP_OK) {
      return false;
    }

    _delay_us = (uint32_t)((1e6f / _freq) / 2.0f + 0.5f);
    if (_delay_us < 1) _delay_us = 1;

    return true;
  }

  bool SoftI2C_Class::release(void) const
  {
    if (_pin_sda >= 0) {
      gpio_set_direction((gpio_num_t)_pin_sda, GPIO_MODE_INPUT);
    }
    if (_pin_scl >= 0) {
      gpio_set_direction((gpio_num_t)_pin_scl, GPIO_MODE_INPUT);
    }
    return true;
  }

  void SoftI2C_Class::_delay() const
  {
    esp_rom_delay_us(_delay_us);
  }

  void SoftI2C_Class::_scl_high() const
  {
    gpio_set_level((gpio_num_t)_pin_scl, 1);
  }

  void SoftI2C_Class::_scl_low() const
  {
    gpio_set_level((gpio_num_t)_pin_scl, 0);
  }

  void SoftI2C_Class::_sda_high() const
  {
    gpio_set_level((gpio_num_t)_pin_sda, 1);
  }

  void SoftI2C_Class::_sda_low() const
  {
    gpio_set_level((gpio_num_t)_pin_sda, 0);
  }

  bool SoftI2C_Class::_sda_read() const
  {
    return gpio_get_level((gpio_num_t)_pin_sda) == 1;
  }

  bool SoftI2C_Class::_i2c_start() const
  {
    _sda_high();
    _scl_high();
    _delay();

    _sda_low();
    _delay();

    _scl_low();
    _delay();

    return true;
  }

  bool SoftI2C_Class::_i2c_stop() const
  {
    _sda_low();
    _delay();

    _scl_high();
    _delay();

    _sda_high();
    _delay();

    return true;
  }

  bool SoftI2C_Class::_i2c_write_byte(uint8_t data) const
  {
    for (int i = 7; i >= 0; i--) {
      if (data & (1 << i)) {
        _sda_high();
      } else {
        _sda_low();
      }
      _delay();

      _scl_high();
      _delay();

      _scl_low();
      _delay();
    }

    _sda_high();
    _delay();

    _scl_high();
    _delay();

    bool ack = !_sda_read();

    _scl_low();
    _delay();

    return ack;
  }

  uint8_t SoftI2C_Class::_i2c_read_byte(bool ack) const
  {
    uint8_t data = 0;

    _sda_high();

    for (int i = 7; i >= 0; i--) {
      _scl_high();
      _delay();

      if (_sda_read()) {
        data |= (1 << i);
      }

      _scl_low();
      _delay();
    }

    if (ack) {
      _sda_low();
    } else {
      _sda_high();
    }
    _delay();

    _scl_high();
    _delay();
    _scl_low();
    _delay();

    return data;
  }

  bool SoftI2C_Class::start(std::uint8_t address, bool read, std::uint32_t freq) const
  {
    if (_pin_sda < 0 || _pin_scl < 0) {
      return false;
    }

    if (freq > 0) {
      _delay_us = (uint32_t)((1e6f / freq) / 2.0f + 0.5f);
      if (_delay_us < 1) _delay_us = 1;
    }

    if (!_i2c_start()) {
      return false;
    }

    uint8_t address_byte = (address << 1) | (read ? 1 : 0);
    if (!_i2c_write_byte(address_byte)) {
      _i2c_stop();
      return false;
    }

    return true;
  }

  bool SoftI2C_Class::restart(std::uint8_t address, bool read, std::uint32_t freq) const
  {
    return start(address, read, freq);
  }

  bool SoftI2C_Class::stop(void) const
  {
    if (_pin_sda < 0 || _pin_scl < 0) {
      return false;
    }

    return _i2c_stop();
  }

  bool SoftI2C_Class::write(std::uint8_t data) const
  {
    if (_pin_sda < 0 || _pin_scl < 0) {
      return false;
    }

    return _i2c_write_byte(data);
  }

  bool SoftI2C_Class::write(const std::uint8_t* data, std::size_t length) const
  {
    if (_pin_sda < 0 || _pin_scl < 0) {
      return false;
    }

    if (!data || length == 0) {
      return true;
    }

    for (std::size_t i = 0; i < length; i++) {
      if (!_i2c_write_byte(data[i])) {
        return false;
      }
    }

    return true;
  }

  bool SoftI2C_Class::read(std::uint8_t* result, std::size_t length, bool last_nack) const
  {
    if (_pin_sda < 0 || _pin_scl < 0) {
      return false;
    }

    if (!result || length == 0) {
      return true;
    }

    for (std::size_t i = 0; i < length; i++) {
      bool send_ack = (i != length - 1) || !last_nack;
      result[i] = _i2c_read_byte(send_ack);
    }

    return true;
  }

  bool SoftI2C_Class::writeRegister(std::uint8_t address, std::uint8_t reg, const std::uint8_t* data, std::size_t length, std::uint32_t freq) const
  {
    if (_pin_sda < 0 || _pin_scl < 0) {
      return false;
    }

    if (freq > 0) {
      _delay_us = (uint32_t)((1e6f / freq) / 2.0f + 0.5f);
      if (_delay_us < 1) _delay_us = 1;
    }

    if (!_i2c_start()) {
      return false;
    }

    uint8_t address_byte = (address << 1) | 0;
    if (!_i2c_write_byte(address_byte)) {
      _i2c_stop();
      return false;
    }

    if (!_i2c_write_byte(reg)) {
      _i2c_stop();
      return false;
    }

    for (std::size_t i = 0; i < length; i++) {
      if (!_i2c_write_byte(data[i])) {
        _i2c_stop();
        return false;
      }
    }

    // 发送 STOP 信号
    _i2c_stop();
    return true;
  }

  bool SoftI2C_Class::readRegister(std::uint8_t address, std::uint8_t reg, std::uint8_t* result, std::size_t length, std::uint32_t freq) const
  {
    if (_pin_sda < 0 || _pin_scl < 0) {
      return false;
    }

    if (freq > 0) {
      _delay_us = (uint32_t)((1e6f / freq) / 2.0f + 0.5f);
      if (_delay_us < 1) _delay_us = 1;
    }

    if (!_i2c_start()) {
      return false;
    }

    uint8_t address_byte = (address << 1) | 0;
    if (!_i2c_write_byte(address_byte)) {
      _i2c_stop();
      return false;
    }

    if (!_i2c_write_byte(reg)) {
      _i2c_stop();
      return false;
    }

    if (!_i2c_start()) {
      return false;
    }

    address_byte = (address << 1) | 1;
    if (!_i2c_write_byte(address_byte)) {
      _i2c_stop();
      return false;
    }

    for (std::size_t i = 0; i < length; i++) {
      bool send_ack = (i != length - 1);
      result[i] = _i2c_read_byte(send_ack);
    }

    _i2c_stop();
    return true;
  }

  bool SoftI2C_Class::writeRegister8(std::uint8_t address, std::uint8_t reg, std::uint8_t data, std::uint32_t freq) const
  {
    return writeRegister(address, reg, &data, 1, freq);
  }

  std::uint8_t SoftI2C_Class::readRegister8(std::uint8_t address, std::uint8_t reg, std::uint32_t freq) const
  {
    std::uint8_t result = 0;
    if (!readRegister(address, reg, &result, 1, freq)) {
      return 0;
    }
    return result;
  }

  bool SoftI2C_Class::bitOn(std::uint8_t address, std::uint8_t reg, std::uint8_t data, std::uint32_t freq) const
  {
    std::uint8_t current = readRegister8(address, reg, freq);
    if (current == 0 && !readRegister(address, reg, &current, 1, freq)) {
      return false;
    }
    return writeRegister8(address, reg, current | data, freq);
  }

  bool SoftI2C_Class::bitOff(std::uint8_t address, std::uint8_t reg, std::uint8_t data, std::uint32_t freq) const
  {
    std::uint8_t current = readRegister8(address, reg, freq);
    if (current == 0 && !readRegister(address, reg, &current, 1, freq)) {
      return false;
    }
    return writeRegister8(address, reg, current & ~data, freq);
  }

  void SoftI2C_Class::scanID(bool* result, std::uint32_t freq) const
  {
    if (!result) {
      return;
    }

    std::memset(result, false, 120);

    if (_pin_sda < 0 || _pin_scl < 0) {
      return;
    }

    if (freq > 0) {
      _delay_us = (uint32_t)((1e6f / freq) / 2.0f + 0.5f);
      if (_delay_us < 1) _delay_us = 1;
    }

    for (int addr = 0x03; addr < 0x78; addr++) {
      if (scanID(addr, freq)) {
        result[addr] = true;
      }
    }
  }

  bool SoftI2C_Class::scanID(uint8_t addr, std::uint32_t freq) const
  {
    if (_pin_sda < 0 || _pin_scl < 0) {
      return false;
    }

    if (freq > 0) {
      _delay_us = (uint32_t)((1e6f / freq) / 2.0f + 0.5f);
      if (_delay_us < 1) _delay_us = 1;
    }

    if (!_i2c_start()) {
      return false;
    }

    uint8_t address_byte = (addr << 1) | 0;
    bool ack = _i2c_write_byte(address_byte);

    _i2c_stop();

    return ack;
  }

} // namespace m5