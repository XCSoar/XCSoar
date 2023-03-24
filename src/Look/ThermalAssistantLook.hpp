// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"

struct ThermalAssistantLook {
  static constexpr Color background_color = COLOR_WHITE;
  static constexpr Color circle_color{0xB0, 0xB0, 0xB0};
  static constexpr Color text_color = COLOR_BLACK;
  static constexpr Color polygon_fill_color{0xCC, 0xCC, 0xFF};
  static constexpr Color polygon_border_color = COLOR_BLUE;

  Brush polygon_brush;

  Pen plane_pen, polygon_pen;
  Pen inner_circle_pen;
  Pen outer_circle_pen;

  Font circle_label_font, overlay_font;

  void Initialise(bool small, bool inverse);
};
