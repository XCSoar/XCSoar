// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Pen.hpp"

class Font;

struct ButtonLook {
  const Font *font;

  struct StateLook {
    Color foreground_color;
    Brush foreground_brush;

    Color background_color;
    Pen light_border_pen, dark_border_pen;
    Brush light_border_brush, dark_border_brush;

    void CreateBorder(Color light, Color dark) {
      light_border_pen.Create(1, light);
      light_border_brush.Create(light);
      dark_border_pen.Create(1, dark);
      dark_border_brush.Create(dark);
    }
  } standard, selected, focused;

  struct {
    Color color;
    Brush brush;
  } disabled;

  void Initialise(const Font &_font);
};
