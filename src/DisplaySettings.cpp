// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplaySettings.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

void
DisplaySettings::SetDefaults()
{
  orientation = DisplayOrientation::DEFAULT;
  cursor_size = 1;
  invert_cursor_colors = false;
#if defined(__APPLE__) && TARGET_OS_IPHONE
  /* iOS has always laid out XCSoar inside the safe area; keep that,
     or an update would silently move every existing installation
     behind the status bar and the display cutout */
  full_screen = false;
  safe_area_stretch = SAFE_AREA_STRETCH_NONE;
#else
  full_screen = true;
  /* Android has always drawn the InfoBoxes and gauges edge to edge */
  safe_area_stretch = SAFE_AREA_STRETCH_ALL;
#endif
  status_bar = StatusBar::AUTO;
#ifdef KOBO
  display_type = DisplayType::E_INK;
#else
  display_type = DisplayType::LCD;
#endif
}
