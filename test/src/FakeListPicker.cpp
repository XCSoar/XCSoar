// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/ComboPicker.hpp"

bool
ComboPicker([[maybe_unused]] const char *caption,
            [[maybe_unused]] DataField &df,
            [[maybe_unused]] const char *help_text,
            [[maybe_unused]] const char *extra_caption,
            bool *extra_selected)
{
  if (extra_selected != nullptr)
    *extra_selected = false;
  return false;
}
