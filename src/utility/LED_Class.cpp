// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "LED_Class.hpp"

namespace m5
{
  bool LED_Class::begin(void)
  {
    bool result = _initialized;
    if (!result)
    {
      if (_led_instance)
      {
        if (_led_instance->begin())
        {
          result = true;
          _initialized = result;
        }
      }
    }
    return result;
  }

  size_t LED_Class::getCount(void) const
  {
    size_t count = 0;
    if (_led_instance)
    {
      count = _led_instance->getCount();
    }
    return count;
  }

  void LED_Class::setBrightness(uint8_t brightness)
  {
    if (begin())
    {
      _led_instance->setBrightness(brightness);
    }
  }

  void LED_Class::setAllColor(const m5gfx::rgb888_t& rgb888)
  {
    if (begin())
    {
      auto count = _led_instance->getCount(); 
      for (size_t i = 0; i < count; i++)
      {
        _led_instance->setColors(&rgb888, i, 1);
      }
    }
  }

  void LED_Class::setColor(size_t index, const m5gfx::rgb888_t& rgb888)
  {
    if (begin())
    {
      _led_instance->setColors(&rgb888, index, 1);
    }
  }

  void LED_Class::display(void)
  {
    if (begin())
    {
      _led_instance->display();
    }
  }
}
