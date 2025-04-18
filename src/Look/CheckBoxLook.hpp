// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Pen.hpp"

class Font;

struct CheckBoxLook {
  const Font *font;

  Brush focus_background_brush;

  struct StateLook {
    Brush box_brush;
    Pen box_pen;

    Brush check_brush;

    Color text_color;
  } standard, focused, pressed, disabled;

  void Initialise(const Font &_font);
};
