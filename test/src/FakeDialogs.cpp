// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Message.hpp"
#include "Dialogs/DataField.hpp"

int
ShowMessageBox([[maybe_unused]] const TCHAR *lpText,
               [[maybe_unused]] const TCHAR *lpCaption,
               [[maybe_unused]] unsigned uType) noexcept
{
  return -1;
}

bool
EditDataFieldDialog([[maybe_unused]] const TCHAR *caption,
                    [[maybe_unused]] DataField &df,
                    [[maybe_unused]] const TCHAR *help_text)
{
  return false;
}
