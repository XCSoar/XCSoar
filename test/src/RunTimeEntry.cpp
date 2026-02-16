// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/TimeEntry.hpp"
#include "time/RoughTime.hpp"

#include <stdio.h>

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  RoughTime value = RoughTime::Invalid();
  const RoughTimeDelta time_zone = RoughTimeDelta::FromMinutes(0);
  if (!TimeEntryDialog("The caption", value, time_zone, true))
    return;

  if (value.IsValid())
    printf("%02u:%02u\n", value.GetHour(), value.GetMinute());
  else
    printf("invalid\n");
}
