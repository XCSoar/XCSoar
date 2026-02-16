// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/NumberEntry.hpp"
#include "Math/Angle.hpp"

#include <stdio.h>

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  Angle value = Angle::Zero();
  if (!AngleEntryDialog("The caption", value))
    return;

  printf("%ld\n", lround(value.Degrees()));
}
