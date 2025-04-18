// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConditionMonitorSunset.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Math/SunEphemeris.hpp"
#include "Computer/Settings.hpp"

using namespace std::chrono;

bool
ConditionMonitorSunset::CheckCondition(const NMEAInfo &basic,
                                       const DerivedInfo &calculated,
                                       const ComputerSettings &settings) noexcept
{
  if (!basic.location_available ||
      !basic.time_available || !basic.date_time_utc.IsDatePlausible() ||
      !calculated.flight.flying || !basic.gps.real ||
      !calculated.task_stats.task_valid)
    return false;

  const GlideResult& res = calculated.task_stats.total.solution_remaining;
  if (!res.IsOk())
    return false;

  /// @todo should be destination location

  SunEphemeris::Result sun =
    SunEphemeris::CalcSunTimes(basic.location, basic.date_time_utc,
                               settings.utc_offset);

  const auto time_local = basic.time + settings.utc_offset.ToDuration();

  using Hours = duration<double, hours::period>;
  const auto d1 = (time_local + res.time_elapsed).Cast<Hours>();
  const auto d0 = time_local.Cast<Hours>();

  bool past_sunset = (d1.count() > sun.time_of_sunset) && (d0.count() < sun.time_of_sunset);
  return past_sunset;
}

void
ConditionMonitorSunset::Notify() noexcept
{
  Message::AddMessage(_("Expect arrival past sunset"));
}
