// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TopographyLook.hpp"
#include "FontDescription.hpp"
#include "Screen/Layout.hpp"

void
TopographyLook::Initialise()
{
  regular_label_font.Load(FontDescription(Layout::FontScale(8), false, false));
  important_label_font.Load(FontDescription(Layout::FontScale(8), true, false));
}
