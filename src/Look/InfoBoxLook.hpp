// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"
#include "util/Macros.hpp"

class Font;

struct InfoBoxLook {
  static constexpr unsigned BORDER_WIDTH = 1;

  bool inverse;

  Pen border_pen;
  Color background_color, focused_background_color, pressed_background_color;

  /**
   * Used only by #InfoBoxSettings::BorderStyle::SHADED.
   */
  Color caption_background_color;

  struct {
    Color fg_color;
  } title, value, comment;

  Font value_font, small_value_font;

  /**
   * The font for units.
   */
  Font unit_font;

  Pen unit_fraction_pen;

  Font title_font;

  Color colors[6];

  void Initialise(bool inverse, bool use_colors,
                  unsigned width, unsigned scale_title_font);

  void ReinitialiseLayout(unsigned width, unsigned scale_title_font);

  Color GetColor(int i, Color default_color) const {
    if (i < 0)
      return colors[0];
    else if (i >= 1 && (unsigned)i < ARRAY_SIZE(colors))
      return colors[i];
    else
      return default_color;
  }

  Color GetTitleColor(int i) const {
    return GetColor(i, title.fg_color);
  }

  Color GetValueColor(int i) const {
    return GetColor(i, value.fg_color);
  }

  Color GetCommentColor(int i) const {
    return GetColor(i, comment.fg_color);
  }
};
