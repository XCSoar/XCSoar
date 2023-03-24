// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

struct HorizonLook {
  Pen aircraft_pen;

  static constexpr Color sky_color{0x0a, 0xb9, 0xf3};
  Brush sky_brush;
  Pen sky_pen;

  static constexpr Color terrain_color{0x80, 0x45, 0x15};
  Brush terrain_brush;
  Pen terrain_pen;

  void Initialise();
};
