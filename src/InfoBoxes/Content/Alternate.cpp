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

#include "InfoBoxes/Content/Alternate.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Util/Gradient.hpp"
#include "Dialogs/Dialogs.h"
#include "MainWindow.hpp"

#include <stdio.h>
#include <tchar.h>

void
InfoBoxContentAlternateName::Update(InfoBoxWindow &infobox)
{
  if (protected_task_manager == NULL) {
    infobox.SetInvalid();
    return;
  }

  const Waypoint *way_point;
  const GlideResult *solution;

  {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    const AbortTask::AlternateVector &alternates = lease->getAlternates();

    if (!alternates.empty()) {
      if (index >= alternates.size())
        index = alternates.size() - 1;

      way_point = &alternates[index].first;
      solution = &alternates[index].second;
    } else {
      way_point = NULL;
      solution = NULL;
    }
  }

  TCHAR tmp[32];
  _stprintf(tmp, _T("Altrn %d"), index+1);
  infobox.SetTitle(tmp);

  SetCommentFromWaypointName(infobox, way_point);
  if (!way_point || !XCSoarInterface::Basic().track_available) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  Angle Value = solution->Vector.Bearing -
    XCSoarInterface::Basic().track;

  SetValueBearingDifference(infobox, Value);

  // Set Color (blue/black)
  infobox.SetColor(solution->glide_reachable(true) ? 2 : 0);
}

bool
InfoBoxContentAlternateName::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    dlgAlternatesListShowModal(XCSoarInterface::main_window);
    break;
  case ibkLeft:
    if (index > 0)
      index--;
    break;
  case ibkRight:
    index++;
    break;
  default:
    return false;
  }

  return true;
}

void
InfoBoxContentAlternateGR::Update(InfoBoxWindow &infobox)
{
  if (protected_task_manager == NULL) {
    infobox.SetInvalid();
    return;
  }

  const Waypoint *way_point;
  const GlideResult *solution;

  {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    const AbortTask::AlternateVector &alternates = lease->getAlternates();

    if (!alternates.empty()) {
      if (index >= alternates.size())
        index = alternates.size() - 1;

      way_point = &alternates[index].first;
      solution = &alternates[index].second;
    } else {
      way_point = NULL;
      solution = NULL;
    }
  }

  TCHAR tmp[32];
  _stprintf(tmp, _T("Altrn %d GR"), index+1);
  infobox.SetTitle(tmp);

  SetCommentFromWaypointName(infobox, way_point);
  if (!way_point) {
    infobox.SetInvalid();
    return;
  }

  fixed gradient = ::AngleToGradient(solution->destination_angle_ground());

  if (negative(gradient)) {
    infobox.SetColor(0);
    infobox.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    TCHAR tmp[32];
    _stprintf(tmp, _T("%d"), (int)gradient);
    infobox.SetValue(tmp);
  } else {
    infobox.SetInvalid();
  }

  // Set Color (blue/black)
  infobox.SetColor(solution->glide_reachable(true) ? 2 : 0);
}

bool
InfoBoxContentAlternateGR::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    dlgAlternatesListShowModal(XCSoarInterface::main_window);
    break;
  case ibkLeft:
    if (index > 0)
      index--;
    break;
  case ibkRight:
    index++;
    break;
  default:
    return false;
  }

  return true;
}
