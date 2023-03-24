// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TerminalLook.hpp"
#include "FontDescription.hpp"
#include "Screen/Layout.hpp"

void
TerminalLook::Initialise()
{
  font.Load(FontDescription(Layout::FontScale(11), false, false, true));
}
