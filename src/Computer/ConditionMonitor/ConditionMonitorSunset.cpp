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

  const double sunset = SunEphemeris::NormalizeHourOfDay(sun.time_of_sunset);
  const double t0 = SunEphemeris::NormalizeHourOfDay(d0.count());
  const double t1 = SunEphemeris::NormalizeHourOfDay(d1.count());

  /* Detect crossing the sunset time-of-day; this works even if the
     local time range crosses midnight (t1 wraps to a smaller value). */
  const bool past_sunset = t1 >= t0
    ? (t0 < sunset && t1 >= sunset)
    : (t0 < sunset || t1 >= sunset);

  return past_sunset;
}

void
ConditionMonitorSunset::Notify() noexcept
{
  Message::AddMessage(_("Expect arrival past sunset"));
}
