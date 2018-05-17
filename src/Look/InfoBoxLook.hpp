/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_INFO_BOX_LOOK_HPP
#define XCSOAR_INFO_BOX_LOOK_HPP

#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Font.hpp"
#include "Util/Macros.hpp"

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
                  unsigned width);

  void ReinitialiseLayout(unsigned width);

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

#endif
