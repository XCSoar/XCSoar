// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UIState.hpp"

void
UIState::Clear()
{
  screen_blanked = false;
  force_display_mode = DisplayMode::NONE;
  display_mode = DisplayMode::NONE;
  auxiliary_enabled = false;
  panel_index = 0;
  panel_name.clear();
  pages.Clear();
  weather.Clear();
}
