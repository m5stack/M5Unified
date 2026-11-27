// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_IMPLEMENTATION
#error "Buzzer_Class.inl is part of M5Unified.cpp and is not meant to be included on its own"
#endif

#include "Buzzer_Class.hpp"
#include "Speaker_Class.hpp"
#include <cmath>

#if defined (CONFIG_IDF_TARGET_ESP32C61)
#include "M5IOE1_Class.hpp"
#include <esp_timer.h>
#endif

namespace m5
{
#if defined (CONFIG_IDF_TARGET_ESP32C61)
  static constexpr uint8_t buzzer_stop_retries = 3;
  static constexpr int64_t buzzer_stop_retry_us = 20000;
#endif

  Buzzer_Class::~Buzzer_Class()
  {
#if defined (CONFIG_IDF_TARGET_ESP32C61)
    if (_ioe1 && (_ioe1_powered || _ioe1_playing.load()))
    {
      std::lock_guard<std::mutex> lock(_ioe1_mutex);
      _powerDownIOE1Locked();
    }
#endif
  }

  bool Buzzer_Class::isEnabled() const
  {
#if defined (CONFIG_IDF_TARGET_ESP32C61)
    if (_ioe1) { return true; }
#endif
    return _speaker && _speaker->isEnabled();
  }

  bool Buzzer_Class::isPlaying() const
  {
#if defined (CONFIG_IDF_TARGET_ESP32C61)
    if (_ioe1) { return _ioe1_playing.load(); }
#endif
    return _speaker && _speaker->_is_buzzer_playing();
  }

  bool Buzzer_Class::tone(float frequency, uint32_t duration)
  {
    if (duration == 0 || !std::isfinite(frequency) || frequency < 1) { return false; }

    bool use_pwm = false;
#if defined (CONFIG_IDF_TARGET_ESP32C61)
    use_pwm = _ioe1 != nullptr;
#endif
    if (use_pwm)
    {
      frequency = std::round(frequency);
      if (frequency > UINT16_MAX) { return false; }
    }
    else if (frequency >= 262144 || !isEnabled()) { return false; }

    uint32_t repeat = UINT32_MAX;
    if (duration != UINT32_MAX)
    {
      const double requested_periods = static_cast<double>(frequency) * duration / 1000;
      const double periods = use_pwm ? std::floor(requested_periods) : std::round(requested_periods);
      if (periods > UINT32_MAX - 1) { return false; }
      repeat = periods < 1 ? 1 : static_cast<uint32_t>(periods);
    }

#if defined (CONFIG_IDF_TARGET_ESP32C61)
    if (use_pwm) { return _startIOE1(static_cast<uint16_t>(frequency), duration); }
#endif

    static constexpr uint8_t square[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    };
    // Keep the input rate near the output rate (down to 4 points) to bound the mixer cost.
    size_t points = sizeof(square);
    while (points > 4 && frequency * points > _speaker->_cfg.sample_rate) { points >>= 1; }
    return _speaker->_play_buzzer(&square[(sizeof(square) - points) >> 1], points, frequency * points, repeat);
  }

  void Buzzer_Class::stop()
  {
#if defined (CONFIG_IDF_TARGET_ESP32C61)
    if (_ioe1)
    {
      std::lock_guard<std::mutex> lock(_ioe1_mutex);
      if (!_sleep_depth) { _stopIOE1Locked(); }
      return;
    }
#endif
    if (_speaker) { _speaker->_stop_channel(Speaker_Class::buzzer_channel); }
  }

#if defined (CONFIG_IDF_TARGET_ESP32C61)
  void Buzzer_Class::_attachIOE1(M5IOE1_Class& ioe1)
  {
    // The expander stays powered across MCU reset. Clear both retained outputs.
    _ioe1_pwm_needs_disable = !ioe1.setPwmDuty12bit(M5IOE1_Class::pwm_ch3, 0, pwm_polarity_t::normal, false);
    const bool power_cut = ioe1.digitalWrite(M5IOE1_Class::gpio2, false);
    _ioe1 = &ioe1;
    if (_ioe1_pwm_needs_disable && !power_cut)
    {
      std::lock_guard<std::mutex> lock(_ioe1_mutex);
      _ioe1_playing.store(true);
      _scheduleStopRetriesLocked();
    }
  }

  Buzzer_Class::SleepGuard::SleepGuard(Buzzer_Class& buzzer) : _buzzer(buzzer)
  {
    if (!_buzzer._ioe1) { return; }
    std::lock_guard<std::mutex> lock(_buzzer._ioe1_mutex);
    if (_buzzer._sleep_depth++ == 0) { _buzzer._powerDownIOE1Locked(); }
  }

  Buzzer_Class::SleepGuard::~SleepGuard()
  {
    if (!_buzzer._ioe1) { return; }
    std::lock_guard<std::mutex> lock(_buzzer._ioe1_mutex);
    --_buzzer._sleep_depth;
  }

  void Buzzer_Class::_powerDownIOE1Locked()
  {
    _deadline_us = 0;
    // Sleep can start as soon as this returns. Retry here if both writes fail.
    for (unsigned attempt = 0; attempt <= buzzer_stop_retries; ++attempt)
    {
      const bool pwm_disabled = _disablePwmLocked();
      const bool power_cut = _cutPowerLocked();
      if (pwm_disabled || power_cut) { return; }
    }
    // update() can retry if sleep is cancelled or execution resumes.
    _scheduleStopRetriesLocked();
  }

  bool Buzzer_Class::_startIOE1(uint16_t frequency, uint32_t duration)
  {
    const bool finite = duration != UINT32_MAX;
    std::lock_guard<std::mutex> lock(_ioe1_mutex);
    if (_sleep_depth) { return false; }

    if (!_ioe1_powered)
    {
      // A power cutoff can leave an enabled PWM behind. Clear it before
      // restoring the rail so a failed start cannot revive the old tone.
      if (_ioe1_pwm_needs_disable && !_disablePwmLocked()) { return false; }
      if (!_ioe1->setHighImpedance(M5IOE1_Class::gpio2, false)
       || !_ioe1->setDirection(M5IOE1_Class::gpio2, true)
       || !_ioe1->setHighImpedance(M5IOE1_Class::gpio11, false)
       || !_ioe1->setDirection(M5IOE1_Class::gpio11, true)
       || !_ioe1->digitalWrite(M5IOE1_Class::gpio2, true)) { return false; }
      _ioe1_powered = true;
    }
    if (!_ioe1->setPwmFrequency(frequency)) { return false; }
    if (!_ioe1->setPwmDuty12bit(M5IOE1_Class::pwm_ch3, 2048))
    {
      if (!_ioe1_playing.exchange(true))
      {
        // A failed transaction can leave PWM enabled. If this was idle,
        // recover it to idle instead of leaving an untracked tone running.
        _stopIOE1Locked();
      }
      return false;
    }

    _deadline_us = 0;
    if (finite)
    {
      const int64_t requested_us = static_cast<int64_t>(duration) * 1000;
      const int64_t period_us = (1000000 + frequency - 1) / frequency;
      _deadline_us = esp_timer_get_time() + (requested_us < period_us ? period_us : requested_us);
      _stop_attempts = buzzer_stop_retries + 1;
    }
    _ioe1_playing.store(true);
    return true;
  }

  bool Buzzer_Class::_disablePwmLocked()
  {
    _ioe1_pwm_needs_disable = !_ioe1->setPwmDuty12bit(M5IOE1_Class::pwm_ch3, 0, pwm_polarity_t::normal, false);
    if (_ioe1_pwm_needs_disable) { return false; }
    _ioe1_playing.store(false);
    return true;
  }

  bool Buzzer_Class::_cutPowerLocked()
  {
    const bool power_cut = _ioe1->digitalWrite(M5IOE1_Class::gpio2, false);
    if (power_cut) { _ioe1_playing.store(false); }
    // Reassert the rail on the next tone, even if the cutoff was unconfirmed.
    _ioe1_powered = false;
    return power_cut;
  }

  void Buzzer_Class::_stopIOE1Locked()
  {
    _deadline_us = 0;
    if (!_disablePwmLocked() && _ioe1_playing.load())
    {
      _scheduleStopRetriesLocked();
    }
  }

  void Buzzer_Class::_scheduleStopRetriesLocked()
  {
    _stop_attempts = buzzer_stop_retries;
    _deadline_us = esp_timer_get_time() + buzzer_stop_retry_us;
  }

  void Buzzer_Class::update()
  {
    if (!_ioe1) { return; }
    std::lock_guard<std::mutex> lock(_ioe1_mutex);
    // Same lock as tone(), so a newer tone is not stopped by an old deadline.
    if (_sleep_depth || !_deadline_us || esp_timer_get_time() < _deadline_us) { return; }

    if (_disablePwmLocked())
    {
      _deadline_us = 0;
    }
    else if (--_stop_attempts == 0)
    {
      _cutPowerLocked();
      _deadline_us = 0;
    }
    else
    {
      _deadline_us = esp_timer_get_time() + buzzer_stop_retry_us;
    }
  }
#endif
}
