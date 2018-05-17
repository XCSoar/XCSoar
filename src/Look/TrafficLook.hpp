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

#ifndef XCSOAR_TRAFFIC_LOOK_HPP
#define XCSOAR_TRAFFIC_LOOK_HPP

#include "Screen/Color.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Icon.hpp"

class Font;

struct TrafficLook {
  Color safe_color;
  Color warning_color;
  Color alarm_color;

  Brush safe_brush;
  Brush warning_brush;
  Brush alarm_brush;

  static constexpr Color team_color_green = Color(0x74, 0xff, 0);
  static constexpr Color team_color_magenta = Color(0xff, 0, 0xcb);
  static constexpr Color team_color_blue = Color(0, 0x90, 0xff);
  static constexpr Color team_color_yellow = Color(0xff, 0xe8, 0);

  Pen team_pen_green;
  Pen team_pen_blue;
  Pen team_pen_yellow;
  Pen team_pen_magenta;

  MaskedIcon teammate_icon;

  const Font *font;

  void Initialise(const Font &font);
};

#endif
