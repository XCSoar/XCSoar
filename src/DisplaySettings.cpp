// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplaySettings.hpp"

#ifdef KOBO
#include "Kobo/Model.hpp"

DisplayType
GetKoboDefaultDisplayType(KoboModel model) noexcept
{
  return model == KoboModel::CLARA_COLOUR
    ? DisplayType::COLOR_E_INK
    : DisplayType::E_INK;
}
#endif

void
DisplaySettings::SetDefaults()
{
  orientation = DisplayOrientation::DEFAULT;
  cursor_size = 1;
  invert_cursor_colors = false;
  full_screen = true;
#ifdef KOBO
  display_type = GetKoboDefaultDisplayType(DetectKoboModel());
#else
  display_type = DisplayType::LCD;
#endif
}
