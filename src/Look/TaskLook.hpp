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

#ifndef XCSOAR_TASK_LOOK_HPP
#define XCSOAR_TASK_LOOK_HPP

#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Features.hpp"

struct TaskLook {
  Pen oz_current_pen;
  Pen oz_active_pen;
  Pen oz_inactive_pen;

  Pen leg_active_pen;
  Pen leg_inactive_pen;
  Pen arrow_active_pen;
  Pen arrow_inactive_pen;
  Pen isoline_pen;

  Pen bearing_pen;
  Pen best_cruise_track_pen;
  Brush best_cruise_track_brush;

  Pen highlight_pen;

  MaskedIcon target_icon;

  /**
   * Used by TaskProgressRenderer.
   */
  Brush hbGray, hbGreen, hbOrange, hbLightGray, hbNotReachableTerrain;

  void Initialise();
};

#endif
