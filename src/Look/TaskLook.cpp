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

#include "TaskLook.hpp"
#include "Screen/Layout.hpp"
#include "Resources.hpp"
#include "Colors.hpp"

void
TaskLook::Initialise()
{
  // Magenta ICAO color is 0x65,0x23,0x1c
  const Color task_color = Color(0x62, 0x4e, 0x90);
  const Color bearing_color = Color(0x3e, 0x30, 0x5f);
  const Color isoline_color = bearing_color;

  oz_current_pen.Create(Pen::SOLID, Layout::ScalePenWidth(2), task_color);
  oz_active_pen.Create(Pen::SOLID, Layout::ScalePenWidth(1), task_color);
  oz_inactive_pen.Create(Pen::SOLID, Layout::ScalePenWidth(1),
                      DarkColor(task_color));

  leg_active_pen.Create(Pen::DASH2, Layout::ScalePenWidth(2), task_color);
  leg_inactive_pen.Create(Pen::DASH2, Layout::ScalePenWidth(1), task_color);
  arrow_active_pen.Create(Layout::ScalePenWidth(2), task_color);
  arrow_inactive_pen.Create(Layout::ScalePenWidth(1), task_color);

  isoline_pen.Create(Pen::DASH2, Layout::ScalePenWidth(1), isoline_color);

  bearing_pen.Create(Layout::ScalePenWidth(2),
                  HasColors() ? bearing_color : COLOR_BLACK);
  best_cruise_track_brush.Create(ColorWithAlpha(bearing_color, ALPHA_OVERLAY));
  best_cruise_track_pen.Create(Layout::ScalePenWidth(1),
                               HasColors()
                               ? DarkColor(bearing_color)
                               : COLOR_BLACK);

  highlight_pen.Create(Layout::ScalePenWidth(4), COLOR_BLACK);

  target_icon.LoadResource(IDB_TARGET, IDB_TARGET_HD);

  hbGray.Create(COLOR_GRAY);
  hbGreen.Create(COLOR_GREEN);
  hbOrange.Create(COLOR_ORANGE);
  hbLightGray.Create(COLOR_LIGHT_GRAY);
  hbNotReachableTerrain.Create(LightColor(COLOR_RED));
}
