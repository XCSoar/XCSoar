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

#include "FlarmTrafficLook.hpp"
#include "Look/TrafficLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"

void
FlarmTrafficLook::Initialise(const TrafficLook &other, bool small)
{
  passive_color = Color(0x99, 0x99, 0x99);
  warning_color = other.warning_color;
  alarm_color = other.alarm_color;
  default_color = COLOR_BLACK;
  selection_color = COLOR_BLUE;
  background_color = COLOR_WHITE;
  radar_color = COLOR_LIGHT_GRAY;

  warning_brush.set(warning_color);
  alarm_brush.set(alarm_color);
  default_brush.set(default_color);
  passive_brush.set(passive_color);
  selection_brush.set(selection_color);
  radar_brush.set(radar_color);
  team_brush_green.set(Color(0x74, 0xFF, 0));
  team_brush_blue.set(Color(0, 0x90, 0xFF));
  team_brush_yellow.set(Color(0xFF, 0xE8, 0));
  team_brush_magenta.set(Color(0xFF, 0, 0xCB));

  int width = Layout::FastScale(small ? 1 : 2);
  warning_pen.set(width, warning_color);
  alarm_pen.set(width, alarm_color);
  default_pen.set(width, default_color);
  passive_pen.set(width, passive_color);
  selection_pen.set(width, selection_color);
  team_pen_green.set(width, Color(0x74, 0xFF, 0));
  team_pen_blue.set(width, Color(0, 0x90, 0xFF));
  team_pen_yellow.set(width, Color(0xFF, 0xE8, 0));
  team_pen_magenta.set(width, Color(0xFF, 0, 0xCB));

  plane_pen.set(width, radar_color);
  radar_pen.set(1, radar_color);

  no_traffic_font.set(Fonts::GetStandardFontFace(), Layout::FastScale(24));
  label_font.set(Fonts::GetStandardFontFace(), Layout::FastScale(14));
  side_info_font.set(Fonts::GetStandardFontFace(),
                 Layout::FastScale(small ? 12 : 18), true);

  info_labels_font.set(Fonts::GetStandardFontFace(), Layout::FastScale(10), true);
  info_values_font.set(Fonts::GetStandardFontFace(), Layout::FastScale(20));
  call_sign_font.set(Fonts::GetStandardFontFace(), Layout::FastScale(28), true);
}

void
FlarmTrafficLook::Deinitialise()
{
  warning_brush.reset();
  alarm_brush.reset();
  default_brush.reset();
  passive_brush.reset();
  selection_brush.reset();
  radar_brush.reset();
  team_brush_green.reset();
  team_brush_blue.reset();
  team_brush_yellow.reset();
  team_brush_magenta.reset();

  warning_pen.reset();
  alarm_pen.reset();
  default_pen.reset();
  passive_pen.reset();
  selection_pen.reset();
  team_pen_green.reset();
  team_pen_blue.reset();
  team_pen_yellow.reset();
  team_pen_magenta.reset();

  plane_pen.reset();
  radar_pen.reset();

  no_traffic_font.reset();
  label_font.reset();
  side_info_font.reset();

  info_labels_font.reset();
  info_values_font.reset();
  call_sign_font.reset();
}
