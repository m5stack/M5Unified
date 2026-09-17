// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_PCF8563_CLASS_H__
#define __M5_PCF8563_CLASS_H__

#include "RTC_Base.hpp"

namespace m5
{
  class PCF8563_Class : public RTC_Base
  {
  public:

    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x51;

    PCF8563_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : RTC_Base ( i2c_addr, freq, i2c )
    {}

    bool begin(I2C_Class* i2c = nullptr) override;

    bool getDateTime(rtc_date_t* date, rtc_time_t* time) const override;
    bool setDateTime(const rtc_date_t* date, const rtc_time_t* time) override;

    bool setTimerIRQ(std::uint32_t timer_msec, std::uint32_t* applied_msec = nullptr) override;

    bool hasTimerIRQ(void) const override { return true; }
    bool canSetAlarm(const rtc_date_t *date, const rtc_time_t *time) const override { return validateAlarmFields(date, time); }
    /// Set alarm by time
    bool setAlarmIRQ(const rtc_date_t *date, const rtc_time_t *time) override;

    bool getIRQstatus(void) override;
    bool clearIRQ(void) override;
    bool disableIRQ(void) override;

    bool getVoltLow(void) override;
  };
}

#endif
