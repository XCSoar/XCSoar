// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

struct HorizonLook {
  Pen aircraft_pen;
  Brush aircraft_brush;
  Pen mark_pen;
  Brush mark_brush;

  Pen horizon_pen;

  static constexpr Color sky_color{0x13, 0x9c, 0xff};
  Brush sky_brush;
  Pen sky_pen;

  static constexpr Color terrain_color{0x78, 0x51, 0x2a};
  Brush terrain_brush;
  Pen terrain_pen;

  void Initialise();
};
