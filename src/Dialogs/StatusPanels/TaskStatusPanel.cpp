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
#include "Form/Edit.hpp"
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

  WndProperty *wp;
  TCHAR Temp[80];

  wp = (WndProperty*)form.FindByName(_T("prpTaskTime"));
  Units::TimeToTextHHMMSigned(Temp, (int)protected_task_manager->GetOrderedTaskBehaviour().aat_min_time);
  assert(wp != NULL);
  if (task_stats.has_targets)
    wp->SetText(Temp);
  else
    wp->hide();

  wp = (WndProperty*)form.FindByName(_T("prpETETime"));
  assert(wp != NULL);
  int ete_time(task_stats.total.time_elapsed +
               task_stats.total.time_remaining);
  Units::TimeToTextHHMMSigned(Temp, ete_time);
  wp->SetText(Temp);

  wp = (WndProperty*)form.FindByName(_T("prpRemainingTime"));
  assert(wp != NULL);
  Units::TimeToTextHHMMSigned(Temp, (int)task_stats.total.time_remaining);
  wp->SetText(Temp);

  if (task_stats.total.planned.IsDefined()) {
    wp = (WndProperty*)form.FindByName(_T("prpTaskDistance"));
    assert(wp != NULL);
    Units::FormatUserDistance(task_stats.total.planned.get_distance(),
                              Temp, ARRAY_SIZE(Temp));
    wp->SetText(Temp);
  }

  if (task_stats.total.remaining.IsDefined()) {
    wp = (WndProperty*)form.FindByName(_T("prpRemainingDistance"));
    assert(wp != NULL);
    Units::FormatUserDistance(task_stats.total.remaining.get_distance(),
                              Temp, ARRAY_SIZE(Temp));
    wp->SetText(Temp);
  }

  if (task_stats.total.planned.IsDefined()) {
    wp = (WndProperty*)form.FindByName(_T("prpEstimatedSpeed"));
    assert(wp != NULL);
    if (task_stats.total.planned.IsDefined())
      Units::FormatUserTaskSpeed(task_stats.total.planned.get_speed(),
                                 Temp, ARRAY_SIZE(Temp));
    wp->SetText(Temp);
  }

  if (task_stats.total.travelled.IsDefined()) {
    wp = (WndProperty*)form.FindByName(_T("prpAverageSpeed"));
    assert(wp != NULL);
    Units::FormatUserTaskSpeed(task_stats.total.travelled.get_speed(),
                               Temp, ARRAY_SIZE(Temp));
    wp->SetText(Temp);
  }
}

void
TaskStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_TASK"));
}
