/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "TaskStatusPanel.hpp"
#include "Util/Macros.hpp"
#include "Interface.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Language/Language.hpp"

enum Controls {
  TaskTime,
  ETETime,
  RemainingTime,
  TaskDistance,
  RemainingDistance,
  EstimatedSpeed,
  AverageSpeed,
};

void
TaskStatusPanel::Refresh()
{
  if (protected_task_manager == nullptr)
    return;

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;

  TCHAR buffer[80];

  SetRowVisible(TaskTime, task_stats.has_targets);
  if (task_stats.has_targets) {
    FormatSignedTimeHHMM(buffer,
                         (int)protected_task_manager->GetOrderedTaskSettings().aat_min_time);
    SetText(TaskTime, buffer);
  }

  int ete_time(task_stats.GetEstimatedTotalTime());
  FormatSignedTimeHHMM(buffer, ete_time);
  SetText(ETETime, buffer);

  FormatSignedTimeHHMM(buffer, (int)task_stats.total.time_remaining_now);
  SetText(RemainingTime, buffer);

  if (task_stats.total.planned.IsDefined()) {
    FormatUserDistanceSmart(task_stats.total.planned.GetDistance(),
                            buffer, ARRAY_SIZE(buffer));
    SetText(TaskDistance, buffer);
  } else
    ClearText(TaskDistance);

  if (task_stats.total.remaining.IsDefined()) {
    FormatUserDistanceSmart(task_stats.total.remaining.GetDistance(),
                            buffer, ARRAY_SIZE(buffer));
    SetText(RemainingDistance, buffer);
  }

  if (task_stats.total.planned.IsDefined()) {
    FormatUserTaskSpeed(task_stats.total.planned.GetSpeed(),
                        buffer, ARRAY_SIZE(buffer));
    SetText(EstimatedSpeed, buffer);
  } else
    ClearText(EstimatedSpeed);

  if (task_stats.total.travelled.IsDefined()) {
    FormatUserTaskSpeed(task_stats.total.travelled.GetSpeed(),
                        buffer, ARRAY_SIZE(buffer));
    SetText(AverageSpeed, buffer);
  } else
    ClearText(AverageSpeed);
}

void
TaskStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddReadOnly(_("Assigned task time"));
  AddReadOnly(_("Estimated task time"));
  AddReadOnly(_("Remaining time"));
  AddReadOnly(_("Task distance"));
  AddReadOnly(_("Remaining distance"));
  AddReadOnly(_("Speed estimated"));
  AddReadOnly(_("Speed average"));
}
