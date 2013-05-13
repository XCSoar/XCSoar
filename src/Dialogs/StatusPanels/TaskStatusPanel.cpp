/*
Copyright_License {

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
  if (protected_task_manager == NULL)
    return;

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const CommonStats &common_stats = calculated.common_stats;

  TCHAR Temp[80];

  SetRowVisible(TaskTime, common_stats.ordered_has_targets);
  if (common_stats.ordered_has_targets) {
    FormatSignedTimeHHMM(Temp, (int)protected_task_manager->GetOrderedTaskSettings().aat_min_time);
    SetText(TaskTime, Temp);
  }

  int ete_time(task_stats.GetEstimatedTotalTime());
  FormatSignedTimeHHMM(Temp, ete_time);
  SetText(ETETime, Temp);

  FormatSignedTimeHHMM(Temp, (int)task_stats.total.time_remaining);
  SetText(RemainingTime, Temp);

  if (task_stats.total.planned.IsDefined()) {
    FormatUserDistanceSmart(task_stats.total.planned.GetDistance(),
                              Temp, ARRAY_SIZE(Temp));
    SetText(TaskDistance, Temp);
  } else
    SetText(TaskDistance, _T(""));

  if (task_stats.total.remaining.IsDefined()) {
    FormatUserDistanceSmart(task_stats.total.remaining.GetDistance(),
                              Temp, ARRAY_SIZE(Temp));
    SetText(RemainingDistance, Temp);
  }

  if (task_stats.total.planned.IsDefined()) {
    FormatUserTaskSpeed(task_stats.total.planned.GetSpeed(),
                               Temp, ARRAY_SIZE(Temp));
    SetText(EstimatedSpeed, Temp);
  } else
    SetText(EstimatedSpeed, _T(""));

  if (task_stats.total.travelled.IsDefined()) {
    FormatUserTaskSpeed(task_stats.total.travelled.GetSpeed(),
                               Temp, ARRAY_SIZE(Temp));
    SetText(AverageSpeed, Temp);
  } else
    SetText(AverageSpeed, _T(""));
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
