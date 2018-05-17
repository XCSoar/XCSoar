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

#include "IconLook.hpp"
#include "Resources.hpp"

void
IconLook::Initialise()
{
  hBmpTabTask.LoadResource(IDB_TASK, IDB_TASK_HD);
  hBmpTabWrench.LoadResource(IDB_WRENCH, IDB_WRENCH_HD);
  hBmpTabSettings.LoadResource(IDB_SETTINGS, IDB_SETTINGS_HD);
  hBmpTabCalculator.LoadResource(IDB_CALCULATOR, IDB_CALCULATOR_HD);

  hBmpTabFlight.LoadResource(IDB_GLOBE, IDB_GLOBE_HD);
  hBmpTabSystem.LoadResource(IDB_DEVICE, IDB_DEVICE_HD);
  hBmpTabRules.LoadResource(IDB_RULES, IDB_RULES_HD);
  hBmpTabTimes.LoadResource(IDB_CLOCK, IDB_CLOCK_HD);
}
