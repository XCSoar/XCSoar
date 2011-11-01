/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Util/Macros.hpp"

class Font;

struct InfoBoxLook {
  static const unsigned BORDER_WIDTH = 1;

  bool inverse;

  Pen border_pen, selector_pen;
  Brush background_brush;

  struct {
    Color fg_color;
    const Font *font;
  } title, value, comment;

  const Font *small_font;
#ifndef GNAV
  const Font *unit_font;
#endif

  Color colors[6];

  void Initialise(bool inverse, bool use_colors);

  Color get_color(int i, Color default_color) const {
    if (i < 0)
      return colors[0];
    else if (i >= 1 && (unsigned)i < ARRAY_SIZE(colors))
      return colors[i];
    else
      return default_color;
  }

  Color get_title_color(int i) const {
    return get_color(i, title.fg_color);
  }

  Color get_value_color(int i) const {
    return get_color(i, value.fg_color);
  }

  Color get_comment_color(int i) const {
    return get_color(i, comment.fg_color);
  }
};

#endif
