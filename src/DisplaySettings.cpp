// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplaySettings.hpp"

void
DisplaySettings::SetDefaults()
{
  orientation = DisplayOrientation::DEFAULT;
  cursor_size = 1;
  invert_cursor_colors = false;
  full_screen = true;
}
