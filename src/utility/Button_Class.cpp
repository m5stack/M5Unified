// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Button_Class.hpp"

namespace m5
{
  void Button_Class::setState(std::uint32_t msec, button_state_t state)
  {
    _lastMsec = msec;
    bool flg_timeout = (msec - _lastClicked > _msecHold);
    switch (state)
    {
    case state_nochange:
      if (flg_timeout && !_press)
      {
        switch (_changeState)
        {
        case state_nochange:
          if (_clickCount) { state = state_decide_click_count; }
          break;

        default:
          _clickCount = 0;
          break;
        }
      }
      break;

    case state_clicked:
      ++_clickCount;
      _lastClicked = msec;
      break;

    case state_decide_click_count:
      _clickCount = 0;
      break;

    default:
      break;
    }
    _changeState = state;
  }

  void Button_Class::setRawState(std::uint32_t msec, bool press)
  {
    button_state_t state = button_state_t::state_nochange;
    bool disable_db = (msec - _lastMsec) > _msecDebounce;
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
          state = button_state_t::state_hold;
        }
      }
      else
      {
        _press = 0;
        if (_oldPress == 1)
        {
          state = button_state_t::state_clicked;
        }
      }
    }
    setState(msec, state);
  }
}
