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

#include "AirspaceWarningMonitor.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Audio/Sound.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Event/Idle.hpp"

void
AirspaceWarningMonitor::Reset()
{
  const auto &calculated = CommonInterface::Calculated();

  last = calculated.airspace_warnings.latest;
}

void
AirspaceWarningMonitor::Check()
{
  const auto &calculated = CommonInterface::Calculated();

  if (calculated.airspace_warnings.latest == last)
    return;

  /* there's a new airspace warning */

  last = calculated.airspace_warnings.latest;

  auto *airspace_warnings = GetAirspaceWarnings();
  if (airspace_warnings == nullptr)
    return;

  if (dlgAirspaceWarningVisible())
    /* already visible */
    return;

  // un-blank the display, play a sound
  ResetUserIdle();
  PlayResource(_T("IDR_WAV_BEEPBWEEP"));

  // show airspace warnings dialog
  if (CommonInterface::GetUISettings().enable_airspace_warning_dialog)
    dlgAirspaceWarningsShowModal(*airspace_warnings, true);
}
