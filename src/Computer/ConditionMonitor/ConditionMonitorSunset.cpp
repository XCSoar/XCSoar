/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}

*/

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
