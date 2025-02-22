// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"

class Font;

struct NavigatorLook {
  bool inverse;

  // const Font *font_map;

  Font waypoint_font, small_value_font, unit_font;

  static constexpr Color color_background_frame{COLOR_WHITE};
  static constexpr Color color_background_frame_inv{COLOR_BLACK};
  static constexpr Color color_frame{COLOR_BLACK};
  static constexpr Color color_frame_inv{COLOR_WHITE};

  Pen pen_frame;
  Brush brush_frame;

  // static constexpr Color sky_color{0x0a, 0xb9, 0xf3};
  // Brush sky_brush;
  // Pen sky_pen;

  // static constexpr Color terrain_color{0x80, 0x45, 0x15};
  // Brush terrain_brush;
  // Pen terrain_pen;

  void Initialise(bool _inverse);
  // void ReinitialiseLayout(unsigned width, unsigned scale_title_font);
};
