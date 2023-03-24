// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
