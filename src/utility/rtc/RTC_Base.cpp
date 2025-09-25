// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

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
}
