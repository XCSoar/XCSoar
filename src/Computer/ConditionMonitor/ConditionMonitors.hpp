// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ConditionMonitorAATTime.hpp"
#include "ConditionMonitorFinalGlide.hpp"
#include "ConditionMonitorGlideTerrain.hpp"
#include "ConditionMonitorLandableReachable.hpp"
#include "ConditionMonitorSunset.hpp"
#include "ConditionMonitorWind.hpp"

class ConditionMonitors {
  ConditionMonitorWind wind;
  ConditionMonitorFinalGlide finalglide;
  ConditionMonitorSunset sunset;
  ConditionMonitorAATTime aattime;
  ConditionMonitorGlideTerrain glideterrain;
  ConditionMonitorLandableReachable landablereachable;

  bool suppressed = false;

public:
  /**
   * Silence all six monitors.
   *
   * Used while a Resume sweep replays hours of old fixes, which would
   * otherwise bury the pilot in stale notifications the moment the progress
   * dialog closes.
   *
   * The flag lives here rather than at the call site so that
   * GlideComputer::ProcessGPS stays byte-identical: it runs for every fix of
   * every live flight, and is the one place where a conditional could break
   * flights that already work.
   */
  void SetSuppressed(bool _suppressed) noexcept {
    suppressed = _suppressed;
  }

  void Update(const NMEAInfo &basic, const DerivedInfo &calculated,
              const ComputerSettings &settings) noexcept;
};
