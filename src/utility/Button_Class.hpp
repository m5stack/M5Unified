// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_BUTTON_CLASS_H__
#define __M5_BUTTON_CLASS_H__

#include <cstdint>

namespace m5
{
  class Button_Class
  {
  public:
    /// Returns true when the button is pressed briefly and released.
    bool wasClicked(void)  const { return _changeState == 1; }

    /// Returns true when the button has been held pressed for a while.
    bool wasHold(void)     const { return _changeState == 2; }

    /// Returns true if the button is currently held pressed.
    bool isHolding(void)   const { return _oldPress == 2 && _press == 2; }
    bool wasChangePressed(void)  const { return ((bool)_press) != ((bool)_oldPress); }

    bool isPressed(void)   const { return _press; }
    bool isReleased(void)  const { return !_press; }
    bool wasPressed(void)  const { return !_oldPress && _press; }
    bool wasReleased(void) const { return _oldPress && !_press; }
    bool pressedFor(std::uint32_t ms)  const { return (_press  && _lastMsec - _lastChange >= ms); }
    bool releasedFor(std::uint32_t ms) const { return (!_press && _lastMsec - _lastChange >= ms); }


    void setDebounceThresh(std::uint32_t msec) { _msecDebounce = msec; }
    void setHoldThresh(std::uint32_t msec) { _msecHold = msec; }

    void setRawState(std::uint32_t msec, bool press);
    void setState(std::uint32_t msec, std::uint8_t state);
    std::uint32_t lastChange(void) const { return _lastChange; }

  private:
    std::uint32_t _lastMsec = 0;
    std::uint32_t _lastChange = 0;
    std::uint32_t _lastRawChange = 0;
    std::uint16_t _msecDebounce = 10;
    std::uint16_t _msecHold = 500;
    bool _raw_press = false;
    std::uint8_t _changeState = 0; // 0:nochange  1:click  2:hold
    std::uint8_t _press = 0;     // 0:release  1:click  2:holding
    std::uint8_t _oldPress = 0;
  };

}

#endif
