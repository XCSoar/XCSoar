// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Font.hpp"

class Font;

struct VarioLook {
  bool inverse, colors;

  Color background_color, text_color, dimmed_text_color;

  Color sink_color, lift_color;

  Pen arc_pen, tick_pen;
  Font arc_label_font;

  Brush sink_brush, lift_brush;

  Pen thick_background_pen, thick_sink_pen, thick_lift_pen;

  Bitmap climb_bitmap;

  const Font *text_font;
  Font value_font;

  Font unit_font;
  Pen unit_fraction_pen;

  Font label_font;

  void Initialise(bool inverse, bool colors,
                  unsigned width,
                  const Font &text_font);

  void ReinitialiseLayout(unsigned width);
};
