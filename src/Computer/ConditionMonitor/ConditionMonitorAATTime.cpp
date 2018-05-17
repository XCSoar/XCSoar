/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "NMEA/Derived.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"

bool
ConditionMonitorAATTime::CheckCondition(const NMEAInfo &basic,
                                        const DerivedInfo &calculated,
                                        const ComputerSettings &settings)
{
  if (!calculated.flight.flying ||
      calculated.common_stats.task_type != TaskType::ORDERED ||
      !calculated.ordered_task_stats.task_valid ||
      !calculated.ordered_task_stats.has_targets ||
      !calculated.ordered_task_stats.start.task_started ||
      !calculated.common_stats.active_has_next ||
      calculated.ordered_task_stats.task_finished)
    return false;

  return calculated.ordered_task_stats.total.time_remaining_now <
    calculated.common_stats.aat_time_remaining;
}

void
ConditionMonitorAATTime::Notify()
{
  Message::AddMessage(_("Expect early task arrival"));
}
