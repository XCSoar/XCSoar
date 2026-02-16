// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_MAIN_WINDOW
#define ENABLE_DIALOG
#define ENABLE_DATA_PATH

#include "Main.hpp"
#include "Dialogs/ProfileListDialog.hpp"
#include "Dialogs/DataField.hpp"

bool
EditDataFieldDialog([[maybe_unused]] const char *caption,
                    [[maybe_unused]] DataField &df,
                    [[maybe_unused]] const char *help_text)
{
  return false;
}

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  ProfileListDialog();
}
