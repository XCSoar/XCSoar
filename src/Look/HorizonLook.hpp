// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"

struct HorizonLook {

  static constexpr Color sky_color{0x13, 0x9c, 0xff};
  Brush sky_brush;
  Pen sky_pen;

  static constexpr Color terrain_color{0x78, 0x51, 0x2a};
  Brush terrain_brush;
  Pen terrain_pen;

  static constexpr Color aircraft_color{0xf2, 0xf2, 0x19};
  Pen aircraft_pen;
  Brush aircraft_brush;

  Pen mark_pen;
  Brush mark_brush;
  Font mark_font;

  Pen horizon_pen;

  void Initialise();
};
