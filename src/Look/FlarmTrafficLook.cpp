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

#include "FlarmTrafficLook.hpp"
#include "TrafficLook.hpp"
#include "FontDescription.hpp"
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

  warning_brush.Create(warning_color);
  alarm_brush.Create(alarm_color);
  default_brush.Create(default_color);
  passive_brush.Create(passive_color);
  selection_brush.Create(selection_color);
  radar_brush.Create(radar_color);
  team_brush_green.Create(other.team_color_green);
  team_brush_blue.Create(other.team_color_blue);
  team_brush_yellow.Create(other.team_color_yellow);
  team_brush_magenta.Create(other.team_color_magenta);

  unsigned width = Layout::FastScale(small ? 1u : 2u);
  warning_pen.Create(width, warning_color);
  alarm_pen.Create(width, alarm_color);
  default_pen.Create(width, default_color);
  passive_pen.Create(width, passive_color);
  selection_pen.Create(width, selection_color);
  team_pen_green.Create(width, other.team_color_green);
  team_pen_blue.Create(width, other.team_color_blue);
  team_pen_yellow.Create(width, other.team_color_yellow);
  team_pen_magenta.Create(width, other.team_color_magenta);

  plane_pen.Create(width, radar_color);
  radar_pen.Create(1, radar_color);

  unit_fraction_pen.Create(1, inverse ? COLOR_WHITE : COLOR_BLACK);

  no_traffic_font.Load(FontDescription(Layout::FontScale(22)));
  label_font.Load(FontDescription(Layout::FontScale(12)));
  side_info_font.Load(FontDescription(Layout::FontScale(small ? 8 : 12),
                                      true));

  info_labels_font.Load(FontDescription(Layout::FontScale(12), true));
  info_values_font.Load(FontDescription(Layout::FontScale(16)));
  info_units_font.Load(FontDescription(Layout::FontScale(8)));
  call_sign_font.Load(FontDescription(Layout::FontScale(24), true));
}
