// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/TextEntry.hpp"
#include "util/Macros.hpp"
#include "LocalPath.hpp"
#include "time/RoughTime.hpp"

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  char text[64] = "";
  TextEntryDialog(text, ARRAY_SIZE(text), "The caption");
}
