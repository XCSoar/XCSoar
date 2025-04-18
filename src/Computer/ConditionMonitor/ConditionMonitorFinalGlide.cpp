// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConditionMonitorFinalGlide.hpp"
#include "NMEA/Derived.hpp"
#include "Input/InputQueue.hpp"

bool
ConditionMonitorFinalGlide::CheckCondition([[maybe_unused]] const NMEAInfo &basic,
                                           const DerivedInfo &calculated,
                                           [[maybe_unused]] const ComputerSettings &settings) noexcept
{
  if (!calculated.flight.flying || !calculated.task_stats.task_valid)
    return false;

  const GlideResult &res = calculated.task_stats.total.solution_remaining;

  // TODO: use low pass filter
  tad = res.altitude_difference * 0.2 + 0.8 * tad;

  bool BeforeFinalGlide = !res.IsFinalGlide();

  if (BeforeFinalGlide) {
    Interval_Notification = std::chrono::minutes{5};
    if (tad > 50 && last_tad < -50)
      // report above final glide early
      return true;
    else if (tad < -50)
      last_tad = tad;
  } else {
    Interval_Notification = std::chrono::minutes{1};
    if (res.IsFinalGlide()) {
      if (last_tad < -50 && tad > 1)
        // just reached final glide, previously well below
        return true;

      if (last_tad > 1 && tad < -50) {
        // dropped well below final glide, previously above
        last_tad = tad;
        return true; // JMW this was true before
      }
    }
  }
  return false;
}

void
ConditionMonitorFinalGlide::Notify() noexcept
{
  if (tad > 1)
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_ABOVE);
  if (tad < -1)
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_BELOW);
}

void
ConditionMonitorFinalGlide::SaveLast() noexcept
{
  last_tad = tad;
}
