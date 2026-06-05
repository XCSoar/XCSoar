// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>

/**
 * Low-pass filter for vario display outputs.  Uses a first-order
 * exponential smoother with the device variofil time constant.
 * Updates on every input sample (e.g. 10 Hz PLXVF); GPS time is not
 * used because it advances too slowly for high-rate vario.
 */
class VarioOutputFilter {
  using Clock = std::chrono::steady_clock;

  Clock::time_point last_filter_time{};
  Clock::time_point last_trail_time{};

  bool time_defined = false;

  double period_seconds = 0;
  double output = 0;

public:
  void Reset(double value) noexcept;

  /**
   * Configure filter time constant (seconds).  Zero disables filtering
   * (passthrough).
   */
  void Design(double period_seconds) noexcept;

  /**
   * Update filter state from a new vario sample.
   *
   * @return true when a trail/audio push sample should be emitted
   * (~1 Hz when filtering, every sample when passthrough)
   */
  [[nodiscard]]
  bool Update(double vario) noexcept;

  [[gnu::pure]]
  double GetOutput() const noexcept {
    return output;
  }

  [[gnu::pure]]
  bool IsActive() const noexcept {
    return period_seconds > 0;
  }
};
