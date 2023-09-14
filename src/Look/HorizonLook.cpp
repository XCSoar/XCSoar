// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonLook.hpp"
#include "Screen/Layout.hpp"

void
HorizonLook::Initialise()
{
  aircraft_pen.Create(Layout::Scale(2), COLOR_BLACK);

  sky_brush.Create(sky_color);
  sky_pen.Create(Layout::Scale(1), DarkColor(sky_color));

  terrain_brush.Create(terrain_color);
  terrain_pen.Create(Layout::Scale(1), DarkColor(terrain_color));
}
