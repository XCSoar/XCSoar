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

#include "InfoBoxes/Content/Alternate.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Util/Gradient.hpp"
#include "Dialogs/Dialogs.h"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

#include <stdio.h>
#include <tchar.h>

void
InfoBoxContentAlternateName::Update(InfoBoxData &data)
{
  if (protected_task_manager == NULL) {
    data.SetInvalid();
    return;
  }

  const AbortTask::Alternate *alternate;

  {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    const AbortTask::AlternateVector &alternates = lease->GetAlternates();

    if (!alternates.empty()) {
      if (index >= alternates.size())
        index = alternates.size() - 1;

      alternate = &alternates[index];
    } else {
      alternate = NULL;
    }
  }

  data.FormatTitle(_("Altn %d"), index + 1);

  if (alternate == NULL || !CommonInterface::Basic().track_available) {
    data.SetInvalid();
    return;
  }

  data.SetComment(alternate->waypoint.name.c_str());

  // Set Value
  Angle Value = alternate->solution.vector.bearing -
    CommonInterface::Basic().track;

  data.SetValueFromBearingDifference(Value);

  // Set Color (blue/black)
  data.SetValueColor(alternate->solution.IsFinalGlide() ? 2 : 0);
}

bool
InfoBoxContentAlternateName::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    dlgAlternatesListShowModal(UIGlobals::GetMainWindow());
    break;
  case ibkLeft:
  case ibkUp:
    if (index > 0)
      index--;
    break;
  case ibkRight:
  case ibkDown:
    index++;
    break;
  }

  return true;
}

void
InfoBoxContentAlternateGR::Update(InfoBoxData &data)
{
  if (protected_task_manager == NULL) {
    data.SetInvalid();
    return;
  }

  const AbortTask::Alternate *alternate;

  {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    const AbortTask::AlternateVector &alternates = lease->GetAlternates();

    if (!alternates.empty()) {
      if (index >= alternates.size())
        index = alternates.size() - 1;

      alternate = &alternates[index];
    } else {
      alternate = NULL;
    }
  }

  data.FormatTitle(_T("Altn %d GR"), index + 1);

  if (alternate == NULL) {
    data.SetInvalid();
    return;
  }

  data.SetComment(alternate->waypoint.name.c_str());

  fixed gradient =
    ::AngleToGradient(alternate->solution.DestinationAngleGround());

  if (negative(gradient)) {
    data.SetValueColor(0);
    data.SetValue(_T("+++"));
    return;
  }
  if (::GradientValid(gradient)) {
    data.SetValueFromGlideRatio(gradient);
  } else {
    data.SetInvalid();
  }

  // Set Color (blue/black)
  data.SetValueColor(alternate->solution.IsFinalGlide() ? 2 : 0);
}

bool
InfoBoxContentAlternateGR::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    dlgAlternatesListShowModal(UIGlobals::GetMainWindow());
    break;
  case ibkLeft:
  case ibkUp:
    if (index > 0)
      index--;
    break;
  case ibkRight:
  case ibkDown:
    index++;
    break;
  }

  return true;
}
