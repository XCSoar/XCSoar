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

#include "TaskLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Graphics.hpp"
#include "resource.h"

void
TaskLook::Initialise()
{
  // Magenta ICAO color is 0x65,0x23,0x1c
  const Color task_color = Color(0x62,0x4e,0x90);
  const Color bearing_color = Color(0x3e,0x30,0x5f);
  const Color isoline_color = bearing_color;

  oz_current_pen.Set(Pen::SOLID, Layout::SmallScale(2), task_color);
  oz_active_pen.Set(Pen::SOLID, Layout::SmallScale(1), task_color);
  oz_inactive_pen.Set(Pen::SOLID, Layout::SmallScale(1),
                      DarkColor(task_color));

  leg_active_pen.Set(Pen::DASH, Layout::Scale(2), task_color);
  leg_inactive_pen.Set(Pen::DASH, Layout::Scale(1), task_color);
  arrow_pen.Set(Layout::Scale(1), task_color);

  isoline_pen.Set(Pen::DASH, Layout::Scale(1), isoline_color);

  bearing_pen.Set(Layout::Scale(2), bearing_color);
  best_cruise_track_brush.Set(bearing_color);
  best_cruise_track_pen.Set(Layout::Scale(1), DarkColor(bearing_color));

  target_icon.Load(IDB_TARGET, IDB_TARGET_HD);
}
