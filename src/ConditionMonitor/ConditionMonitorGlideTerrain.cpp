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

#include "ConditionMonitorGlideTerrain.hpp"
#include "Computer/GlideComputer.hpp"
#include "InputEvents.hpp"

bool
ConditionMonitorGlideTerrain::CheckCondition(const GlideComputer& cmp)
{
  if (!cmp.Calculated().flight.flying ||
      !cmp.Calculated().task_stats.task_valid)
    return false;

  const GlideResult& res = cmp.Calculated().task_stats.total.solution_remaining;
  if (!res.IsFinalGlide() || !res.IsAchievable(true))
    // only give message about terrain warnings if above final glide
    return false;

  return cmp.Calculated().terrain_warning;
}

void
ConditionMonitorGlideTerrain::Notify()
{
  InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_TERRAIN);
}
