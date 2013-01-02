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

#include "RulesStatusPanel.hpp"
#include "Util/Macros.hpp"
#include "Logger/Logger.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "LocalTime.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Language/Language.hpp"
#include "Task/ProtectedTaskManager.hpp"
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
  if (protected_task_manager == NULL)
    return;

  TCHAR Temp[80];

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const CommonStats &common_stats = calculated.common_stats;

  /// @todo proper task validity check
  SetText(ValidStart, calculated.common_stats.task_started
          ? _("Yes") : _T("No"));

  SetText(ValidFinish, calculated.common_stats.task_finished
          ? _("Yes") : _T("No"));

  AircraftState start_state = protected_task_manager->GetStartState();

  if (common_stats.task_started) {
    FormatSignedTimeHHMM(Temp, (int)TimeLocal((int)start_state.time));
    SetText(StartTime, Temp);

    FormatUserTaskSpeed(start_state.ground_speed,
                               Temp, ARRAY_SIZE(Temp));
    SetText(StartSpeed, Temp);

    FormatUserAltitude(start_state.altitude, Temp, ARRAY_SIZE(Temp));
    SetText(StartHeight, Temp);
  } else {
    ClearValue(StartTime);
    ClearValue(StartSpeed);
    ClearValue(StartHeight);
  }

  FormatUserAltitude(protected_task_manager->GetFinishHeight(),
                            Temp, ARRAY_SIZE(Temp));
  SetText(FinishAlt, Temp);

  {
    ProtectedTaskManager::Lease task_manager(*protected_task_manager);
    const OrderedTask &task = task_manager->GetOrderedTask();

    if (task.TaskSize() > 0)
      CopyString(Temp, task.GetTaskPoint(0).GetWaypoint().name.c_str(),
                 ARRAY_SIZE(Temp));
    else
      Temp[0] = _T('\0');
  }

  SetText(StartPoint, Temp);
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
