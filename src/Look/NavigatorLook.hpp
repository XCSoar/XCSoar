// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"

class Font;

struct NavigatorLook {
  bool inverse;

  static constexpr Color color_background_frame{COLOR_WHITE};
  static constexpr Color color_background_frame_inv{COLOR_BLACK};
  static constexpr Color color_frame{COLOR_BLACK};
  static constexpr Color color_frame_inv{COLOR_WHITE};

  Pen pen_frame;
  Brush brush_frame;

  void Initialise(bool _inverse);
};
