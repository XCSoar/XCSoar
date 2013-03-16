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

#include "Contest.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/dlgAnalysis.hpp"

#include <tchar.h>

bool
InfoBoxContentOLC::HandleKey(const InfoBoxKeyCodes keycode)
{
  switch (keycode) {
  case ibkEnter:
    dlgAnalysisShowModal(UIGlobals::GetMainWindow(), UIGlobals::GetLook(),
                         CommonInterface::Full(), *glide_computer,
                         protected_task_manager,
                         &airspace_database,
                         terrain, 8);
    return true;

  default:
    break;
  }

  return false;
}

void
InfoBoxContentOLC::Update(InfoBoxData &data)
{
  if (!CommonInterface::GetComputerSettings().contest.enable ||
      !protected_task_manager) {
    data.SetInvalid();
    return;
  }

  const ContestResult& result_olc =
    CommonInterface::Calculated().contest_stats.GetResult();

  if (result_olc.score < fixed(1)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(result_olc.distance);

  data.UnsafeFormatComment(_T("%.1f pts"), (double)result_olc.score);
}
