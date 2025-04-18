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

public:
  void Update(const NMEAInfo &basic, const DerivedInfo &calculated,
              const ComputerSettings &settings) noexcept;
};
