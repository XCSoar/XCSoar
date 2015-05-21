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

  SetRowVisible(TaskTime, task_stats.has_targets);
  if (task_stats.has_targets)
    SetText(TaskTime,
            FormatSignedTimeHHMM((int)protected_task_manager->GetOrderedTaskSettings().aat_min_time));

  SetText(ETETime,
          FormatSignedTimeHHMM((int)task_stats.GetEstimatedTotalTime()));

  SetText(RemainingTime,
          FormatSignedTimeHHMM((int)task_stats.total.time_remaining_now));

  if (task_stats.total.planned.IsDefined())
    SetText(TaskDistance,
            FormatUserDistanceSmart(task_stats.total.planned.GetDistance()));
  else
    ClearText(TaskDistance);

  if (task_stats.total.remaining.IsDefined())
    SetText(RemainingDistance,
            FormatUserDistanceSmart(task_stats.total.remaining.GetDistance()));

  if (task_stats.total.planned.IsDefined())
    SetText(EstimatedSpeed,
            FormatUserTaskSpeed(task_stats.total.planned.GetSpeed()));
  else
    ClearText(EstimatedSpeed);

  if (task_stats.total.travelled.IsDefined())
    SetText(AverageSpeed,
            FormatUserTaskSpeed(task_stats.total.travelled.GetSpeed()));
  else
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
