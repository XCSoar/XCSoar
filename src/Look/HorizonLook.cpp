// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonLook.hpp"
#include "Screen/Layout.hpp"

void
HorizonLook::Initialise()
{
  aircraft_pen.Create(Layout::Scale(2), Color(0xf2, 0xf2, 0x19));
  mark_pen.Create(Layout::Scale(1), COLOR_WHITE);
  horizon_pen.Create(Layout::Scale(1), COLOR_WHITE);

  sky_brush.Create(sky_color);
  // sky_pen.Create(Layout::Scale(1), LightColor(sky_color));
  sky_pen.Create(Layout::Scale(1), COLOR_WHITE);

  terrain_brush.Create(terrain_color);
  // terrain_pen.Create(Layout::Scale(1), LightColor(terrain_color));
  terrain_pen.Create(Layout::Scale(1), COLOR_WHITE);
}
