// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_PWM_TYPES_H__
#define __M5_PWM_TYPES_H__

#include <cstdint>

namespace m5
{
  /// PWM output polarity.
  /// normal   : the duty is the high time. The output idles low  (POL = 0).
  /// inverted : the duty is the low time.  The output idles high (POL = 1),
  ///            which the datasheet calls active low.
  enum class pwm_polarity_t : std::uint8_t
  { normal   = 0
  , inverted = 1
  };
}

#endif
