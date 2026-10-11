// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"
#include "FLARM/SpokenAlert.hpp"
#include "time/Stamp.hpp"

#include <optional>

/**
 * Monitors the FLARM collision alarm and generates haptic feedback
 * when the alarm level rises.  FLARM devices beep on their own, but
 * the sound is easily missed in a noisy cockpit.  Optionally, the
 * alarm is spoken.
 */
class TrafficMonitor {
  FlarmTraffic::AlarmType last = FlarmTraffic::AlarmType::NONE;

  /** The alert spoken last, while the alarm persists */
  std::optional<SpokenTrafficAlert> spoken;

  TimeStamp spoken_time = TimeStamp::Undefined();

public:
  void Reset() noexcept {
    last = FlarmTraffic::AlarmType::NONE;
    spoken.reset();
  }

  void Check() noexcept;

private:
  void Speak(bool escalated) noexcept;
};

/**
 * Play SpokenTrafficAlert::Example(), so the pilot can try the voice
 * and its volume.  Nothing is played while FLARM reports an aircraft
 * alarm: the example would replace what is left of the real alert
 * with a made-up position.
 *
 * @return true if the example plays
 */
bool
PlaySpokenAlertExample() noexcept;
