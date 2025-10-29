// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef M5_LED_BASE_H__
#define M5_LED_BASE_H__

#include <M5GFX.h>

namespace m5
{
  class LED_Base
  {
  public:
    virtual ~LED_Base() {}

    enum led_type_t {
      led_type_unknown = 0,
      led_type_fullcolor,
      led_type_single,
    };

    LED_Base() {}

    virtual bool begin(void) = 0;
    virtual led_type_t getLedType(size_t index) const { return led_type_unknown; }
    virtual size_t getCount(void) const = 0;
    virtual void setColors(const RGBColor* colors, size_t index, size_t length) = 0;
    virtual void setBrightness(const uint8_t brightness) = 0;
    virtual void display(void) = 0;
    virtual RGBColor* getBuffer(void) { return nullptr; }
  };
}

#endif
