// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/DateEntry.hpp"
#include "time/BrokenDate.hpp"

#include <stdio.h>

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  auto value = BrokenDate::TodayUTC();
  if (!DateEntryDialog("The caption", value, true))
    return;

  if (value.IsPlausible())
    printf("%04u-%02u-%02u\n", value.year, value.month, value.day);
  else
    printf("invalid\n");
}
