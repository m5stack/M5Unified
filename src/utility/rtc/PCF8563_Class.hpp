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

    bool getTime(rtc_time_t* time) const override;
    bool getDate(rtc_date_t* date) const override;
    bool getDateTime(rtc_datetime_t* datetime) const override;

    void setTime(const rtc_time_t &time) override;
    void setDate(const rtc_date_t &date) override;

    /// Set timer IRQ
    /// @param afterSeconds 1 - 15,300. If 256 or more, 1-minute cycle.  (max 255 minute.)
    /// @return the set number of seconds.
    int setAlarmIRQ(int afterSeconds);

    /// Set alarm by time
    int setAlarmIRQ(const rtc_time_t &time) override;
    int setAlarmIRQ(const rtc_date_t &date, const rtc_time_t &time) override;

    bool getIRQstatus(void) override;
    void clearIRQ(void) override;
    void disableIRQ(void) override;

    bool getVoltLow(void) override;
  };
}

#endif
