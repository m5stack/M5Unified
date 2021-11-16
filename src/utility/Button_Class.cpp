// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Button_Class.hpp"

namespace m5
{
  void Button_Class::setState(std::uint32_t msec, std::uint8_t state)
  {
    _lastMsec = msec;
    _changeState = state;
  }

  void Button_Class::setRawState(std::uint32_t msec, bool press)
  {
    bool disable_db = (msec - _lastMsec) > _msecDebounce;
    _lastMsec = msec;
    _changeState = 0;
    _oldPress = _press;
    if (_raw_press != press)
    {
      _raw_press = press;
      _lastRawChange = msec;
    }
    if (disable_db || msec - _lastRawChange >= _msecDebounce)
    {
      if (press != (0 != _oldPress))
      {
        _lastChange = msec;
      }
      if (press)
      {
        if (!_oldPress)
        {
          _press = 1;
        } else 
        if (1 == _oldPress && (msec - _lastChange >= _msecHold))
        {
          _press = 2;
          _changeState = 2;
        }
      }
      else
      {
        _press = 0;
        if (_oldPress == 1)
        {
          _changeState = 1;
        }
      }
    }
  }

}
