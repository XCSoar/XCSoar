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

#ifndef FLARM_TRAFFIC_WINDOW_LOOK_HPP
#define FLARM_TRAFFIC_WINDOW_LOOK_HPP

#include "Screen/Color.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Font.hpp"

struct TrafficLook;

struct FlarmTrafficLook {
  Color warning_color;
  Color alarm_color;
  Color default_color;
  Color passive_color;
  Color selection_color;
  Color background_color;
  Color radar_color;

  Brush warning_brush;
  Brush alarm_brush;
  Brush default_brush;
  Brush passive_brush;
  Brush selection_brush;
  Brush radar_brush;
  Brush team_brush_green;
  Brush team_brush_blue;
  Brush team_brush_yellow;
  Brush team_brush_magenta;

  Pen warning_pen;
  Pen alarm_pen;
  Pen default_pen;
  Pen passive_pen;
  Pen selection_pen;

  Pen team_pen_green;
  Pen team_pen_blue;
  Pen team_pen_yellow;
  Pen team_pen_magenta;

  Pen plane_pen, radar_pen;

  Pen unit_fraction_pen;

  Font label_font, side_info_font, no_traffic_font;
  Font info_values_font, info_units_font, info_labels_font, call_sign_font;

  void Initialise(const TrafficLook &other, bool small, bool inverse = false);
};

#endif
