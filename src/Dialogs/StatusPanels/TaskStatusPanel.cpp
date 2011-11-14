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

#include "TaskStatusPanel.hpp"
#include "Util/Macros.hpp"
#include "Interface.hpp"
#include "Form/Util.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"

void
TaskStatusPanel::Refresh()
{
  if (protected_task_manager == NULL)
    return;

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const TaskStats &task_stats = calculated.task_stats;

  TCHAR Temp[80];

  Units::TimeToTextHHMMSigned(Temp, (int)protected_task_manager->GetOrderedTaskBehaviour().aat_min_time);
  ShowFormControl(form, _T("prpTaskTime"), task_stats.has_targets);
  if (task_stats.has_targets)
    SetFormValue(form, _T("prpTaskTime"), Temp);

  int ete_time(task_stats.total.time_elapsed +
               task_stats.total.time_remaining);
  Units::TimeToTextHHMMSigned(Temp, ete_time);
  SetFormValue(form, _T("prpETETime"), Temp);

  Units::TimeToTextHHMMSigned(Temp, (int)task_stats.total.time_remaining);
  SetFormValue(form, _T("prpRemainingTime"), Temp);

  if (task_stats.total.planned.IsDefined()) {
    Units::FormatUserDistance(task_stats.total.planned.get_distance(),
                              Temp, ARRAY_SIZE(Temp));
    SetFormValue(form, _T("prpTaskDistance"), Temp);
  } else
    SetFormValue(form, _T("prpTaskDistance"), _T(""));

  if (task_stats.total.remaining.IsDefined()) {
    Units::FormatUserDistance(task_stats.total.remaining.get_distance(),
                              Temp, ARRAY_SIZE(Temp));
    SetFormValue(form, _T("prpRemainingDistance"), Temp);
  }

  if (task_stats.total.planned.IsDefined()) {
    Units::FormatUserTaskSpeed(task_stats.total.planned.get_speed(),
                               Temp, ARRAY_SIZE(Temp));
    SetFormValue(form, _T("prpEstimatedSpeed"), Temp);
  } else
    SetFormValue(form, _T("prpEstimatedSpeed"), _T(""));

  if (task_stats.total.travelled.IsDefined()) {
    Units::FormatUserTaskSpeed(task_stats.total.travelled.get_speed(),
                               Temp, ARRAY_SIZE(Temp));
    SetFormValue(form, _T("prpAverageSpeed"), Temp);
  } else
    SetFormValue(form, _T("prpAverageSpeed"), _T(""));
}

void
TaskStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_TASK"));
}
