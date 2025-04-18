// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConditionMonitorLandableReachable.hpp"
#include "NMEA/Derived.hpp"
#include "Input/InputQueue.hpp"

bool
ConditionMonitorLandableReachable::CheckCondition([[maybe_unused]] const NMEAInfo &basic,
                                                  const DerivedInfo &calculated,
                                                  [[maybe_unused]] const ComputerSettings &settings) noexcept
{
  if (!calculated.flight.flying)
    return false;

  now_reachable = calculated.common_stats.landable_reachable;

  // warn when becoming unreachable
  return (!now_reachable && last_reachable);
}

void
ConditionMonitorLandableReachable::Notify() noexcept
{
  InputEvents::processGlideComputer(GCE_LANDABLE_UNREACHABLE);
}

void
ConditionMonitorLandableReachable::SaveLast() noexcept
{
  last_reachable = now_reachable;
}
