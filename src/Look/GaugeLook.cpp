// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FontDescription.hpp"



void
GaugeLook::Initialise(const Font &_font)
{
  no_data_font.Load(FontDescription(Layout::FontScale(22)));
}
