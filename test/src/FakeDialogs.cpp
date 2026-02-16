// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Message.hpp"
#include "Dialogs/DataField.hpp"

int
ShowMessageBox([[maybe_unused]] const char *lpText,
               [[maybe_unused]] const char *lpCaption,
               [[maybe_unused]] unsigned uType) noexcept
{
  return -1;
}

bool
EditDataFieldDialog([[maybe_unused]] const char *caption,
                    [[maybe_unused]] DataField &df,
                    [[maybe_unused]] const char *help_text)
{
  return false;
}
