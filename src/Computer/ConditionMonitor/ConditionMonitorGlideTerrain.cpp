// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConditionMonitorGlideTerrain.hpp"
#include "NMEA/Derived.hpp"
#include "Input/InputQueue.hpp"

bool
ConditionMonitorGlideTerrain::CheckCondition([[maybe_unused]] const NMEAInfo &basic,
                                             const DerivedInfo &calculated,
                                             [[maybe_unused]] const ComputerSettings &settings) noexcept
{
  if (!calculated.flight.flying ||
      !calculated.task_stats.task_valid)
    return false;

  const GlideResult& res = calculated.task_stats.total.solution_remaining;
  if (!res.IsFinalGlide())
    // only give message about terrain warnings if above final glide
    return false;

  return calculated.terrain_warning_location.IsValid();
}

void
ConditionMonitorGlideTerrain::Notify() noexcept
{
  InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_TERRAIN);
}
