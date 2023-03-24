// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Fonts.hpp"
#include "Look/FontDescription.hpp"
#include "ui/canvas/Font.hpp"
#include "Screen/Layout.hpp"

Font normal_font, bold_font;

void
InitialiseFonts()
{
  normal_font.Load(FontDescription(Layout::FontScale(12)));
  bold_font.Load(FontDescription(Layout::FontScale(12), true));
}

void
DeinitialiseFonts()
{
  bold_font.Destroy();
  normal_font.Destroy();
}
