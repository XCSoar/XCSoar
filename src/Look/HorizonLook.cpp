// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonLook.hpp"
#include "FontDescription.hpp"
#include "Screen/Layout.hpp"

void
HorizonLook::Initialise()
{
  aircraft_pen.Create(Layout::Scale(1), aircraft_color);
  aircraft_brush.Create(aircraft_color);

  mark_pen.Create(Layout::Scale(1), COLOR_WHITE);
  mark_brush.Create(COLOR_WHITE);
  mark_font.Load(FontDescription(Layout::FontScale(12)));

  horizon_pen.Create(Layout::Scale(1), COLOR_WHITE);

  sky_brush.Create(sky_color);
  sky_pen.Create(Layout::Scale(1), sky_color);

  terrain_brush.Create(terrain_color);
  terrain_pen.Create(Layout::Scale(1), terrain_color);
}
