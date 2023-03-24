// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CrossSectionLook.hpp"

void
CrossSectionLook::Initialise(const Font &_grid_font)
{
  background_color = COLOR_WHITE;
  text_color = COLOR_BLACK;

  sky_color = Color(0xa0, 0xd0, 0xf3);
  terrain_color = Color(0x80, 0x45, 0x15);
  terrain_brush.Create(terrain_color);
  sea_color = Color(0xbd, 0xc5, 0xd5); // ICAO open water area
  sea_brush.Create(sea_color);

  grid_pen.Create(Pen::DASH2, 1, Color(0x60, 0x60, 0x60));
  aircraft_brush.Create(text_color);

  grid_font = &_grid_font;
}
