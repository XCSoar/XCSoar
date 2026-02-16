// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Control.hpp"
#include "Dialogs/HelpDialog.hpp"

WindowControl::WindowControl() noexcept
{
  // Clear the caption
  caption.clear();
}

void
WindowControl::SetCaption(const char *Value) noexcept
{
  if (Value == nullptr)
    Value = "";

  if (!caption.equals(Value)) {
    caption = Value;
    Invalidate();
  }
}

bool
WindowControl::OnHelp() noexcept
{
  if (help_text) {
    HelpDialog(caption.c_str(), help_text);
    return true;
  }

  return false;
}
