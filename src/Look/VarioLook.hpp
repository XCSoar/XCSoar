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

#ifndef XCSOAR_VARIO_LOOK_HPP
#define XCSOAR_VARIO_LOOK_HPP

#include "Screen/Color.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Font.hpp"

class Font;

struct VarioLook {
  bool inverse, colors;

  Color background_color, text_color, dimmed_text_color;

  Color sink_color, lift_color;

  Brush sink_brush, lift_brush;

  Pen thick_background_pen, thick_sink_pen, thick_lift_pen;

  Bitmap background_bitmap;
  unsigned background_x;

  Bitmap climb_bitmap;

  const Font *text_font;
  Font value_font;

  Font unit_font;
  Pen unit_fraction_pen;

  void Initialise(bool inverse, bool colors,
                  const Font &text_font);
};

#endif
