// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Touch_Class.hpp"

namespace m5
{
  void Touch_Class::update(std::uint32_t msec)
  {
    std::size_t count = _gfx->getTouchRaw(_touch_raw, TOUCH_MAX_POINTS);
    _touch_count = count;

    bool updated[TOUCH_MAX_POINTS] = { false };
    if (count)
    {
      m5gfx::touch_point_t tp[TOUCH_MAX_POINTS];
      memcpy(tp, _touch_raw, sizeof(m5gfx::touch_point_t) * count);
      _gfx->convertRawXY(tp, count);
      for (std::size_t i = 0; i < count; ++i)
      {
        if (tp[i].id < TOUCH_MAX_POINTS)
        {
          updated[tp[i].id] = true;
          update_detail(&_touch_detail[tp[i].id], msec, true, &tp[i]);
        }
      }
    }
    for (std::size_t i = 0; i < TOUCH_MAX_POINTS; ++i)
    {
      if ((!updated[i])
       && update_detail(&_touch_detail[i], msec, false, nullptr)
       && (_touch_count < TOUCH_MAX_POINTS))
      {
        _touch_raw[_touch_count].id = i;
        _touch_raw[_touch_count].size = 0;
        _touch_raw[_touch_count].x = -1;
        _touch_raw[_touch_count].y = -1;
        ++_touch_count;
      }
    }
  }

  bool Touch_Class::update_detail(touch_detail_t* det, std::uint32_t msec, bool pressed, m5gfx::touch_point_t* tp)
  {
    touch_state_t tm = det->state;
    tm = static_cast<touch_state_t>(tm & ~touch_state_t::mask_change);
    if (tm) {
      det->prev_x = det->x;
      det->prev_y = det->y;
    }
    if (pressed) {
      det->size = tp->size;
      det->id   = tp->id;
      if (!(tm & touch_state_t::mask_moving))
      { // Processing when not flicked.
        if (tm & touch_state_t::mask_touch)
        { // Not immediately after the touch.
          if (abs(det->base_x - tp->x) > _flickThresh
           || abs(det->base_y - tp->y) > _flickThresh)
          {
            det->prev = det->base;
            tm = static_cast<touch_state_t>(tm | touch_state_t::flick_begin);
          }
          else if (!(tm & touch_state_t::mask_holding))
          { // The hold time has not elapsed.
            if (msec - det->base_msec > _msecHold)
            {
              tm = touch_state_t::hold_begin;
            }
          }
        }
        else
        {
          memcpy((void*)det, tp, sizeof(m5gfx::touch_point_t));
          tm = touch_state_t::touch_begin;
          det->base_msec = msec;
          det->base_x = tp->x;
          det->base_y = tp->y;
          det->prev = det->base;
        }
      }
      if (tm & mask_moving)
      {
        det->x = tp->x;
        det->y = tp->y;
      }
    }
    else if (tm != touch_state_t::none)
    {
      if (tm & touch_state_t::mask_touch)
      {
        tm = static_cast<touch_state_t>((tm | touch_state_t::mask_change) & ~touch_state_t::mask_touch);
      } else {
        tm = touch_state_t::none;
      }
    }
    else
    {
      return false;
    }
    det->state = tm;
    return true;
  }
}
