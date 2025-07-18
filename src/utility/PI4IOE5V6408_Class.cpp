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
void PI4IOE5V6408_Class::setDirection(uint8_t pin, bool direction)
{
    if (direction) {
        bitOn(0x03, 1 << pin); // Output, set 1
    } else {
        bitOff(0x03, 1 << pin); // Input, set 0
    }
}

void PI4IOE5V6408_Class::enablePull(uint8_t pin, bool enablePull)
{
    if (enablePull) {
        bitOn(0x0B, 1 << pin);
    } else {
        bitOff(0x0B, 1 << pin);
    }
}

// false down, true up
void PI4IOE5V6408_Class::setPullMode(uint8_t pin, bool mode)
{
    if (mode) {
        bitOn(0x0D, 1 << pin);
    } else {
        bitOff(0x0D, 1 << pin);
    }
}

void PI4IOE5V6408_Class::setHighImpedance(uint8_t pin, bool enable)
{
    if (enable) {
        bitOn(0x07, 1 << pin);
    } else {
        bitOff(0x07, 1 << pin);
    }
}

bool PI4IOE5V6408_Class::getWriteValue(uint8_t pin)
{
    auto data = readRegister8(0x05);
    return (data & (1 << pin)) != 0;
}

void PI4IOE5V6408_Class::digitalWrite(uint8_t pin, bool level)
{
    if (level) {
        bitOn(0x05, 1 << pin);
    } else {
        bitOff(0x05, 1 << pin);
    }
}

bool PI4IOE5V6408_Class::digitalRead(uint8_t pin)
{
    auto data = readRegister8(0x0F);
    return (data & (1 << pin)) != 0;
}

void PI4IOE5V6408_Class::resetIrq()
{
    readRegister8(0x13);
}

void PI4IOE5V6408_Class::disableIrq()
{
    writeRegister8(0x11, 0B11111111);
}

void PI4IOE5V6408_Class::enableIrq()
{
    writeRegister8(0x11, 0x0);
}
}
