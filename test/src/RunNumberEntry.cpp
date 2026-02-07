// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/NumberEntry.hpp"

#include <stdio.h>

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  unsigned value;
  if (NumberEntryDialog("The caption", value, 6))
    printf("%u\n", value);
}
