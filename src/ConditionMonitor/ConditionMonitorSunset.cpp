/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Computer/GlideComputer.hpp"
#include "Device/device.hpp"
#include "LocalTime.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"

bool
ConditionMonitorSunset::CheckCondition(const GlideComputer& cmp)
{
  if (!cmp.Basic().location_available ||
      !cmp.Calculated().flight.flying || HaveCondorDevice() ||
      !cmp.Calculated().task_stats.task_valid)
    return false;

  const GlideResult& res = cmp.Calculated().task_stats.total.solution_remaining;
  if (!res.IsOk())
    return false;

  /// @todo should be destination location

  sun.CalcSunTimes(cmp.Basic().location, cmp.Basic().date_time_utc,
                   fixed(GetUTCOffset()) / 3600);
  fixed d1((res.time_elapsed + fixed(DetectCurrentTime(cmp.Basic()))) / 3600);
  fixed d0(DetectCurrentTime(cmp.Basic()) / 3600);

  bool past_sunset = (d1 > sun.TimeOfSunSet) && (d0 < sun.TimeOfSunSet);
  return past_sunset;
}

void
ConditionMonitorSunset::Notify(void)
{
  Message::AddMessage(_("Expect arrival past sunset"));
}
