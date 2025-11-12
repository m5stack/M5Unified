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

  RGBColor* LED_Class::getBuffer(void)
  {
    if (!begin()) { return nullptr; }
    return _led_instance->getBuffer();
  }

  void LED_Class::setBrightness(uint8_t brightness)
  {
    if (!begin()) { return; }

    _led_instance->setBrightness(brightness);
    if (_auto_display) { _led_instance->display(); }
  }

  void LED_Class::setAllColor(const RGBColor& rgb888)
  {
    if (!begin()) { return; }

    auto count = _led_instance->getCount(); 
    for (size_t i = 0; i < count; i++)
    {
      _led_instance->setColors(&rgb888, i, 1);
    }
    if (_auto_display) { _led_instance->display(); }
  }

  void LED_Class::setColor(size_t index, const RGBColor& rgb888)
  {
    if (!begin()) { return; }

    _led_instance->setColors(&rgb888, index, 1);
    if (_auto_display) { _led_instance->display(); }
  }

  void LED_Class::_set_colors(size_t index, size_t length, lgfx::pixelcopy_t* pc)
  {
    if (!begin()) { return; }

    int32_t len = static_cast<int32_t>(length);
    int32_t count = _led_instance->getCount();
    if (len > count - (int32_t)index)
    {
      len = count - index;
    }

    std::vector<RGBColor> rgb_buffer(len);
    pc->fp_copy(rgb_buffer.data(), 0, len, pc);
    _led_instance->setColors(rgb_buffer.data(), index, len);

    if (_auto_display) { _led_instance->display(); }
  }


  void LED_Class::display(void)
  {
    if (!begin()) { return; }
    _led_instance->display();
  }
}
