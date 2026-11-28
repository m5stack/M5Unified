// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <stdint.h>

#if defined (ESP_PLATFORM)
#include <sdkconfig.h>
#endif

#if defined (CONFIG_IDF_TARGET_ESP32C61)
#include <atomic>
#include <mutex>
#endif

namespace m5
{
  class M5Unified;
  class M5IOE1_Class;
  class Power_Class;
  class Speaker_Class;

  class Buzzer_Class
  {
  public:
    /// Start a tone, replacing the current one.
    /// @param frequency Hz, at least 1. Speaker: below half the sample rate.
    /// CoreMatrix: rounded to an integer up to 65535.
    /// @param duration Milliseconds. UINT32_MAX plays continuously.
    /// Speaker rounds it to whole periods (at least one).
    /// @return false if rejected. Invalid arguments leave the current tone as is;
    /// a failed I2C write on CoreMatrix may change it.
    /// @note With Speaker, stop recording first on boards that share the mic and
    /// speaker. On CoreMatrix, finite tones are stopped by M5.update().
    /// @note On CoreMatrix, M5.Power sleep and power-off calls stop the tone and
    /// do not resume it. Call tone(), stop() and M5.update() outside open I2C
    /// transactions on the internal bus.
    bool tone(float frequency, uint32_t duration = UINT32_MAX);

    /// Stop the tone. Speaker.stop() does not affect Buzzer; Speaker.end() does.
    void stop();

    /// Whether a tone is playing. With Speaker it can stay true briefly after
    /// stop(). On CoreMatrix a finite tone stays true until M5.update() stops it.
    bool isPlaying() const;

    /// Whether this board has a Buzzer output: the Speaker output configured by
    /// M5.begin() (following later Speaker settings). CoreMatrix always uses its
    /// PWM buzzer, even when a Speaker output is configured.
    bool isEnabled() const;

  private:
    friend class M5Unified;
    friend class Power_Class;

    Buzzer_Class() = default;
    Buzzer_Class(const Buzzer_Class&) = delete;
    Buzzer_Class& operator=(const Buzzer_Class&) = delete;
    ~Buzzer_Class();

    Speaker_Class* _speaker = nullptr;

#if defined (CONFIG_IDF_TARGET_ESP32C61)
    class SleepGuard
    {
    public:
      explicit SleepGuard(Buzzer_Class& buzzer);
      ~SleepGuard();
      SleepGuard(const SleepGuard&) = delete;
      SleepGuard& operator=(const SleepGuard&) = delete;

    private:
      Buzzer_Class& _buzzer;
    };

    void _attachIOE1(M5IOE1_Class& ioe1);
    void update();
    void _powerDownIOE1Locked();
    bool _startIOE1(uint16_t frequency, uint32_t duration);
    void _stopIOE1Locked();
    bool _disablePwmLocked();
    bool _cutPowerLocked();
    void _scheduleStopRetriesLocked();

    M5IOE1_Class* _ioe1 = nullptr;
    std::mutex _ioe1_mutex;
    std::atomic<bool> _ioe1_playing { false };
    int64_t _deadline_us = 0;
    uint8_t _stop_attempts = 0;
    unsigned _sleep_depth = 0;
    bool _ioe1_powered = false;
    bool _ioe1_pwm_needs_disable = false;
#endif
  };
}
