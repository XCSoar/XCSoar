// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/PeriodClock.hpp"

class GlidePolar;

class BallastDumpManager
{
  bool enabled = false;
  PeriodClock ballast_clock;

public:
  bool IsEnabled() const noexcept {
    return enabled;
  }

  void Start() noexcept;
  void Stop() noexcept;

  void SetEnabled(bool _enabled) noexcept;

  /**
   * Updates the ballast of the given GlidePolar.
   * This function must not be called if the BallastDumpManager is not enabled!
   * @param glide_polar The GlidePolar to update
   * @param dump_time Time that it takes to dump the entire ballast
   * @return True if ballast is not entirely dumped yet,
   *         False if we are dry now or dump_time is zero/unknown
   */
  bool Update(GlidePolar &glide_polar, unsigned dump_time) noexcept;
};
