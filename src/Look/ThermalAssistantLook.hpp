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

#ifndef THERMAL_ASSISTANT_WINDOW_LOOK_HPP
#define THERMAL_ASSISTANT_WINDOW_LOOK_HPP

#include "Screen/Color.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Font.hpp"

struct ThermalAssistantLook {
  Color background_color;
  Color circle_color;
  Color text_color;
  Color polygon_fill_color;
  Color polygon_border_color;

  Brush polygon_brush;

  Pen plane_pen, polygon_pen;
  Pen inner_circle_pen;
  Pen outer_circle_pen;

  Font circle_label_font, overlay_font;

  void Initialise(bool small, bool inverse);
};

#endif
