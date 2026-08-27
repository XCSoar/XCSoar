// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"

/**
 * Monitors the FLARM collision alarm and generates haptic feedback
 * when the alarm level rises.  FLARM devices beep on their own, but
 * the sound is easily missed in a noisy cockpit.
 */
class TrafficMonitor {
  FlarmTraffic::AlarmType last = FlarmTraffic::AlarmType::NONE;

public:
  void Reset() noexcept {
    last = FlarmTraffic::AlarmType::NONE;
  }

  void Check() noexcept;
};
