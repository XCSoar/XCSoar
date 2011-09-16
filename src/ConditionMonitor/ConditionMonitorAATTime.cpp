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

#include "ConditionMonitorAATTime.hpp"
#include "Computer/GlideComputer.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"

bool
ConditionMonitorAATTime::CheckCondition(const GlideComputer& cmp)
{
  if (!cmp.Calculated().flight.flying ||
      !cmp.Calculated().task_stats.task_valid ||
      !cmp.Calculated().common_stats.mode_ordered ||
      !cmp.Calculated().common_stats.ordered_valid ||
      !cmp.Calculated().common_stats.ordered_has_targets ||
      !cmp.Calculated().common_stats.task_started ||
      !cmp.Calculated().common_stats.active_has_next ||
      cmp.Calculated().common_stats.task_finished)
    return false;

  if (cmp.Calculated().common_stats.task_time_remaining <
      cmp.Calculated().common_stats.aat_time_remaining)
    return true;
  else
    return false;
}

void
ConditionMonitorAATTime::Notify()
{
  Message::AddMessage(_("Expect early task arrival"));
}
