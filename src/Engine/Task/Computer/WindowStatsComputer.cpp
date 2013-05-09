/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "WindowStatsComputer.hpp"
#include "Task/Stats/TaskStats.hpp"

void
WindowStatsComputer::Compute(fixed time, const TaskStats &task_stats,
                             WindowStats &stats)
{
  if (!task_stats.task_valid || !task_stats.start.task_started ||
      !task_stats.total.travelled.IsDefined()) {
    Reset();
    stats.Reset();
    return;
  }

  if (task_stats.task_finished)
    return;

  const fixed dt = minute_clock.Update(time, fixed(59), fixed(180));
  if (negative(dt)) {
    Reset();
    stats.Reset();
    return;
  }

  if (!positive(dt))
    return;

  travelled_distance.Push(time, task_stats.total.travelled.GetDistance());

  stats.duration = travelled_distance.GetDeltaXChecked();
  if (positive(stats.duration)) {
    stats.distance = travelled_distance.GetDeltaY();
    stats.speed = stats.distance / stats.duration;
  }
}
