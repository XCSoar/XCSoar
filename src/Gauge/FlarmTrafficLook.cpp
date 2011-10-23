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

  warning_brush.Set(warning_color);
  alarm_brush.Set(alarm_color);
  default_brush.Set(default_color);
  passive_brush.Set(passive_color);
  selection_brush.Set(selection_color);
  radar_brush.Set(radar_color);
  team_brush_green.Set(Color(0x74, 0xFF, 0));
  team_brush_blue.Set(Color(0, 0x90, 0xFF));
  team_brush_yellow.Set(Color(0xFF, 0xE8, 0));
  team_brush_magenta.Set(Color(0xFF, 0, 0xCB));

  UPixelScalar width = Layout::FastScale(small ? 1 : 2);
  warning_pen.Set(width, warning_color);
  alarm_pen.Set(width, alarm_color);
  default_pen.Set(width, default_color);
  passive_pen.Set(width, passive_color);
  selection_pen.Set(width, selection_color);
  team_pen_green.Set(width, Color(0x74, 0xFF, 0));
  team_pen_blue.Set(width, Color(0, 0x90, 0xFF));
  team_pen_yellow.Set(width, Color(0xFF, 0xE8, 0));
  team_pen_magenta.Set(width, Color(0xFF, 0, 0xCB));

  plane_pen.Set(width, radar_color);
  radar_pen.Set(1, radar_color);

  no_traffic_font.Set(Fonts::GetStandardFontFace(), Layout::FastScale(24));
  label_font.Set(Fonts::GetStandardFontFace(), Layout::FastScale(14));
  side_info_font.Set(Fonts::GetStandardFontFace(),
                 Layout::FastScale(small ? 12 : 18), true);

  info_labels_font.Set(Fonts::GetStandardFontFace(), Layout::FastScale(10), true);
  info_values_font.Set(Fonts::GetStandardFontFace(), Layout::FastScale(20));
  call_sign_font.Set(Fonts::GetStandardFontFace(), Layout::FastScale(28), true);
}

void
FlarmTrafficLook::Deinitialise()
{
  warning_brush.Reset();
  alarm_brush.Reset();
  default_brush.Reset();
  passive_brush.Reset();
  selection_brush.Reset();
  radar_brush.Reset();
  team_brush_green.Reset();
  team_brush_blue.Reset();
  team_brush_yellow.Reset();
  team_brush_magenta.Reset();

  warning_pen.Reset();
  alarm_pen.Reset();
  default_pen.Reset();
  passive_pen.Reset();
  selection_pen.Reset();
  team_pen_green.Reset();
  team_pen_blue.Reset();
  team_pen_yellow.Reset();
  team_pen_magenta.Reset();

  plane_pen.Reset();
  radar_pen.Reset();

  no_traffic_font.Reset();
  label_font.Reset();
  side_info_font.Reset();

  info_labels_font.Reset();
  info_values_font.Reset();
  call_sign_font.Reset();
}
