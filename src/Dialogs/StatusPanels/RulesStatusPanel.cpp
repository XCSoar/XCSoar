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

#include "RulesStatusPanel.hpp"
#include "Util/Macros.hpp"
#include "Util/TruncateString.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Language/Language.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"

enum Controls {
  ValidStart,
  StartTime,
  StartHeight,
  StartPoint,
  StartSpeed,
  FinishAlt,
  ValidFinish,
};

void
RulesStatusPanel::Refresh()
{
  TCHAR Temp[80];

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.ordered_task_stats;
  const StartStats &start_stats = task_stats.start;
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  /// @todo proper task validity check
  SetText(ValidStart, start_stats.task_started
          ? _("Yes") : _T("No"));

  SetText(ValidFinish, task_stats.task_finished
          ? _("Yes") : _T("No"));

  if (start_stats.task_started) {
    SetText(StartTime,
            FormatLocalTimeHHMM((int)start_stats.time, settings.utc_offset));

    SetText(StartSpeed,
            FormatUserTaskSpeed(start_stats.ground_speed));

    SetText(StartHeight, FormatUserAltitude(start_stats.altitude));
  } else {
    ClearValue(StartTime);
    ClearValue(StartSpeed);
    ClearValue(StartHeight);
  }

  Temp[0] = _T('\0');
  double finish_height(0);

  if (protected_task_manager != nullptr) {
    ProtectedTaskManager::Lease task_manager(*protected_task_manager);
    const OrderedTask &task = task_manager->GetOrderedTask();
    const unsigned task_size = task.TaskSize();

    if (task_size > 0) {
      CopyTruncateString(Temp, ARRAY_SIZE(Temp),
                         task.GetTaskPoint(0).GetWaypoint().name.c_str());
      finish_height = task.GetTaskPoint(task_size - 1).GetElevation();
    }
  }

  SetText(StartPoint, Temp);

  SetText(FinishAlt, FormatUserAltitude(finish_height));
}

void
RulesStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddReadOnly(_("Valid start"));
  AddReadOnly(_("Start time"));
  AddReadOnly(_("Start alt."));
  AddReadOnly(_("Start point"));
  AddReadOnly(_("Start speed"));
  AddReadOnly(_("Finish min. alt."));
  AddReadOnly(_("Valid finish"));
}
