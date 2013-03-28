/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "FlarmTrafficLook.hpp"
#include "TrafficLook.hpp"
#include "StandardFonts.hpp"
#include "Screen/Layout.hpp"

void
FlarmTrafficLook::Initialise(const TrafficLook &other, bool small, bool inverse)
{
  passive_color = Color(0x99, 0x99, 0x99);
  warning_color = other.warning_color;
  alarm_color = other.alarm_color;
  default_color = inverse ? COLOR_WHITE : COLOR_BLACK;
  selection_color = COLOR_BLUE;
  background_color = inverse ? COLOR_BLACK : COLOR_WHITE;
  radar_color = COLOR_GRAY;
  Color team_color_green = Color(0x74, 0xFF, 0);
  Color team_color_blue = Color(0, 0x90, 0xFF);
  Color team_color_yellow = Color(0xFF, 0xE8, 0);
  Color team_color_magenta = Color(0xFF, 0, 0xCB);

  warning_brush.Set(warning_color);
  alarm_brush.Set(alarm_color);
  default_brush.Set(default_color);
  passive_brush.Set(passive_color);
  selection_brush.Set(selection_color);
  radar_brush.Set(radar_color);
  team_brush_green.Set(team_color_green);
  team_brush_blue.Set(team_color_blue);
  team_brush_yellow.Set(team_color_yellow);
  team_brush_magenta.Set(team_color_magenta);

  UPixelScalar width = Layout::FastScale(small ? 1 : 2);
  warning_pen.Set(width, warning_color);
  alarm_pen.Set(width, alarm_color);
  default_pen.Set(width, default_color);
  passive_pen.Set(width, passive_color);
  selection_pen.Set(width, selection_color);
  team_pen_green.Set(width, team_color_green);
  team_pen_blue.Set(width, team_color_blue);
  team_pen_yellow.Set(width, team_color_yellow);
  team_pen_magenta.Set(width, team_color_magenta);

  plane_pen.Set(width, radar_color);
  radar_pen.Set(1, radar_color);

  unit_fraction_pen.Set(1, inverse ? COLOR_WHITE : COLOR_BLACK);

  no_traffic_font.Load(GetStandardFontFace(), Layout::FastScale(24));
  label_font.Load(GetStandardFontFace(), Layout::FastScale(14));
  side_info_font.Load(GetStandardFontFace(),
                      Layout::FastScale(small ? 12 : 18), true);

  info_labels_font.Load(GetStandardFontFace(),
                        Layout::FastScale(10), true);
  info_values_font.Load(GetStandardFontFace(), Layout::FastScale(20));
  info_units_font.Load(GetStandardFontFace(), Layout::FastScale(8));
  call_sign_font.Load(GetStandardFontFace(),
                      Layout::FastScale(28), true);
}
