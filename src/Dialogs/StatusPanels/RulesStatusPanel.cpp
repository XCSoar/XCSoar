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

#include "RulesStatusPanel.hpp"
#include "Util/Macros.hpp"
#include "Logger/Logger.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "LocalTime.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Language/Language.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Form/Edit.hpp"

void
RulesStatusPanel::Refresh()
{
  if (protected_task_manager == NULL)
    return;

  WndProperty *wp;
  TCHAR Temp[80];

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const CommonStats &common_stats = calculated.common_stats;

  wp = (WndProperty*)form.FindByName(_T("prpValidStart"));
  assert(wp != NULL);
  if (calculated.common_stats.task_started)
    /// @todo proper task validity check
    wp->SetText(_("Yes"));
  else
    wp->SetText(_("No"));

  wp = (WndProperty*)form.FindByName(_T("prpValidFinish"));
  assert(wp != NULL);
  if (common_stats.task_finished)
    wp->SetText(_("Yes"));
  else
    wp->SetText(_("No"));

  AircraftState start_state = protected_task_manager->GetStartState();

  wp = (WndProperty*)form.FindByName(_T("prpStartTime"));
  assert(wp != NULL);
  if (common_stats.task_started) {
    Units::TimeToTextHHMMSigned(Temp, (int)TimeLocal((int)start_state.time));
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)form.FindByName(_T("prpStartSpeed"));
  assert(wp != NULL);
  if (common_stats.task_started) {
    Units::FormatUserTaskSpeed(start_state.ground_speed,
                               Temp, ARRAY_SIZE(Temp));
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  // StartMaxHeight, StartMaxSpeed;
  wp = (WndProperty*)form.FindByName(_T("prpStartHeight"));
  assert(wp != NULL);
  if (common_stats.task_started) {
    _stprintf(Temp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(start_state.altitude),
              Units::GetAltitudeName());
    wp->SetText(Temp);
  } else {
    wp->SetText(_T(""));
  }

  wp = (WndProperty*)form.FindByName(_T("prpFinishAlt"));
  assert(wp != NULL);
  _stprintf(Temp, _T("%.0f %s"),
            (double)Units::ToUserAltitude(protected_task_manager->GetFinishHeight()),
            Units::GetAltitudeName());
  wp->SetText(Temp);

  wp = (WndProperty*)form.FindByName(_T("prpStartPoint"));
  assert(wp != NULL);

  TCHAR name[64];
  {
    ProtectedTaskManager::Lease task_manager(*protected_task_manager);
    const OrderedTask &task = task_manager->GetOrderedTask();

    if (task_manager->GetMode() == TaskManager::MODE_ORDERED &&
        task.TaskSize() > 0)
      CopyString(name, task.GetTaskPoint(0)->GetWaypoint().name.c_str(), 64);
    else
      name[0] = _T('\0');
  }

  wp->SetText(name);
}

void
RulesStatusPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent, _T("IDR_XML_STATUS_RULES"));
}
