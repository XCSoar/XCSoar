// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

class Font;

struct NavigatorLook {
  bool inverse;

  const Font *font;

  static constexpr Color background_color{COLOR_WHITE};
  static constexpr Color background_color_inv{COLOR_BLACK};
  static constexpr Color frame_color{COLOR_BLACK};
  static constexpr Color frame_color_inv{COLOR_WHITE};
  
  Pen frame_pen;
  Brush frame_brush;

  Pen background_pen;
  Brush background_brush;

  Pen aircraft_pen;

  static constexpr Color sky_color{0x0a, 0xb9, 0xf3};
  Brush sky_brush;
  Pen sky_pen;

  static constexpr Color terrain_color{0x80, 0x45, 0x15};
  Brush terrain_brush;
  Pen terrain_pen;

  void Initialise(bool _inverse, const Font &_font);
};
