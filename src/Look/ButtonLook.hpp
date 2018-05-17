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

#ifndef XCSOAR_BUTTON_LOOK_HPP
#define XCSOAR_BUTTON_LOOK_HPP

#include "Screen/Color.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Pen.hpp"

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
  } standard, focused;

  struct {
    Color color;
    Brush brush;
  } disabled;

  void Initialise(const Font &_font);
};

#endif
