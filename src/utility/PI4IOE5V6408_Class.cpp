/**
 * @file pi4ioe5v6408.cpp
 * @author Forairaaaaa, lovyan03
 * @brief
 * @version 0.2
 * @date 2025-06-11
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "PI4IOE5V6408_Class.hpp"

namespace m5
{
bool PI4IOE5V6408_Class::begin()
{
    auto id = readRegister8(0x01);
    if (id == 0) return false;
    return true;
}

// false input, true output
bool PI4IOE5V6408_Class::setDirection(uint8_t pin, bool direction)
{
    if (pin >= 8) { return false; }
    if (direction) {
        return bitOn(0x03, 1 << pin); // Output, set 1
    } else {
        return bitOff(0x03, 1 << pin); // Input, set 0
    }
}

bool PI4IOE5V6408_Class::setPullMode(uint8_t pin, gpio_pull_t mode)
{
    if (pin >= 8) return false;
    const auto bit = 1 << pin;
    switch (mode) {
    case pull_none:
        return bitOff(0x0B, bit);
    case pull_up:
        if (!bitOn(0x0D, bit)) { return false; }
        return bitOn(0x0B, bit);
    case pull_down:
        if (!bitOff(0x0D, bit)) { return false; }
        return bitOn(0x0B, bit);
    default:
        return false;
    }
}

bool PI4IOE5V6408_Class::setHighImpedance(uint8_t pin, bool enable)
{
    if (pin >= 8) { return false; }
    if (enable) {
        return bitOn(0x07, 1 << pin);
    } else {
        return bitOff(0x07, 1 << pin);
    }
}

bool PI4IOE5V6408_Class::getWriteValue(uint8_t pin)
{
    auto data = readRegister8(0x05);
    return (data & (1 << pin)) != 0;
}

bool PI4IOE5V6408_Class::digitalWrite(uint8_t pin, bool level)
{
    if (pin >= 8) { return false; }
    if (level) {
        return bitOn(0x05, 1 << pin);
    } else {
        return bitOff(0x05, 1 << pin);
    }
}

bool PI4IOE5V6408_Class::digitalRead(uint8_t pin)
{
    auto data = readRegister8(0x0F);
    return (data & (1 << pin)) != 0;
}

bool PI4IOE5V6408_Class::resetIrq()
{
    uint8_t value;
    return readRegister(0x13, &value, 1);
}

bool PI4IOE5V6408_Class::disableIrq()
{
    return writeRegister8(0x11, 0B11111111);
}

bool PI4IOE5V6408_Class::enableIrq()
{
    return writeRegister8(0x11, 0x0);
}
}
