/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Device/device.hpp"
#include "LocalTime.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "Math/SunEphemeris.hpp"

bool
ConditionMonitorSunset::CheckCondition(const NMEAInfo &basic,
                                       const DerivedInfo &calculated)
{
  if (!basic.location_available ||
      !calculated.flight.flying || HaveCondorDevice() ||
      !calculated.task_stats.task_valid)
    return false;

  const GlideResult& res = calculated.task_stats.total.solution_remaining;
  if (!res.IsOk())
    return false;

  /// @todo should be destination location

  SunEphemeris::Result sun =
    SunEphemeris::CalcSunTimes(basic.location, basic.date_time_utc,
                               fixed(GetUTCOffset()) / 3600);

  fixed d1((res.time_elapsed + fixed(DetectCurrentTime(basic))) / 3600);
  fixed d0(DetectCurrentTime(basic) / 3600);

  bool past_sunset = (d1 > sun.time_of_sunset) && (d0 < sun.time_of_sunset);
  return past_sunset;
}

void
ConditionMonitorSunset::Notify()
{
  Message::AddMessage(_("Expect arrival past sunset"));
}
