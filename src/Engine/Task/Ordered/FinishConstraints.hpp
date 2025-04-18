// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/AltitudeReference.hpp"

struct AircraftState;

struct FinishConstraints {
  /** Minimum height AGL (m) allowed to finish */
  unsigned min_height;

  /** Reference for min finish height */
  AltitudeReference min_height_ref;

  /**
   * Whether ordered task start and finish requires FAI height rules
   * and (no) speed rule.  The default value is
   * TaskFactoryConstraints::fai_finish.
   *
   * When you modify this value, remember to always keep it in sync
   * with StartConstraints::fai_finish!
   */
  bool fai_finish;

  void SetDefaults();

  /**
   * Check whether aircraft height is within finish height limit
   *
   * @param state Aircraft state
   * @param finish_elevation finish point elevation
   *
   * @return True if within limits
   */
  [[gnu::pure]]
  bool CheckHeight(const AircraftState &state,
                   double finish_elevation) const;
};
