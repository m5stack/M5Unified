// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_IMPLEMENTATION
#error "RTC_Base.inl is part of M5Unified.cpp and is not meant to be included on its own"
#endif

#include "RTC_Base.hpp"

#include <stdlib.h>

namespace m5
{
  tm rtc_datetime_t::get_tm(void) const
  {
    tm t_st = {
      time.seconds,
      time.minutes,
      time.hours,
      date.date,
      date.month - 1,
      date.year - 1900,
      date.weekDay,
      0,
      0,
    };
    return t_st;
  }

  void rtc_datetime_t::set_tm(tm& datetime)
  {
    date = rtc_date_t { datetime };
    time = rtc_time_t { datetime };
  }

  bool RTC_Base::validateDateTime(const rtc_date_t* date, const rtc_time_t* time)
  {
    // The range rules live in the structs; a nullptr argument means "not requested".
    return (date ? date->isValid() : true) && (time ? time->isValid() : true);
  }

  bool RTC_Base::validateAlarmFields(const rtc_date_t* date, const rtc_time_t* time)
  {
    if (time && ((time->minutes < -1 || time->minutes > 59)
              || (time->hours < -1 || time->hours > 23)))
    {
      return false;
    }
    return !date || ((date->date == -1 || (date->date >= 1 && date->date <= 31))
                 && (date->weekDay >= -1 && date->weekDay <= 6));
  }
}
