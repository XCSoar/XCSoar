// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IconLook.hpp"
#include "Resources.hpp"

void
IconLook::Initialise()
{
  hBmpTabTask.LoadResource(IDB_TASK_ALL);
  hBmpTabWrench.LoadResource(IDB_WRENCH_ALL);
  hBmpTabSettings.LoadResource(IDB_SETTINGS_ALL);
  hBmpTabCalculator.LoadResource(IDB_CALCULATOR_ALL);

  hBmpTabFlight.LoadResource(IDB_GLOBE_ALL);
  hBmpTabSystem.LoadResource(IDB_DEVICE_ALL);
  hBmpTabRules.LoadResource(IDB_RULES_ALL);
  hBmpTabTimes.LoadResource(IDB_CLOCK_ALL);
}
