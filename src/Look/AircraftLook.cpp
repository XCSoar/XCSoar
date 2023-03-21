// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AircraftLook.hpp"

void
AircraftLook::Initialise()
{
  // Note: No scaling needed. Pens are used with CanvasRotateShift, which
  //       applies Layout::scale.
  aircraft_pen.Create(1, COLOR_DARK_GRAY);
  aircraft_simple1_pen.Create(1, COLOR_BLACK);
  aircraft_simple2_pen.Create(3, COLOR_WHITE);

  canopy_pen.Create(1, DarkColor(COLOR_CYAN));
  canopy_brush.Create(COLOR_CYAN);
}
